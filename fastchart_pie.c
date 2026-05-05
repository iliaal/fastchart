/*
  +----------------------------------------------------------------------+
  | Copyright (c) 2025-2026, Ilia Alshanetsky                            |
  | Copyright (c) 2025-2026, Advanced Internet Designs Inc.              |
  +----------------------------------------------------------------------+
  | This source file is subject to the BSD 3-Clause license that is      |
  | bundled with this package in the file LICENSE.                       |
  +----------------------------------------------------------------------+
  | Author: Ilia Alshanetsky <ilia@ilia.ws>                              |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"
#include "fastchart_effects.h"

int fastchart_pie_render_to_image(fastchart_pie_obj *self, gdImagePtr im)
{
    if (self->slice_count == 0) {
        zend_throw_error(NULL,
            "FastChart\\PieChart::draw() requires setSlices() with one or more positive values");
        return -1;
    }

    /* Render-time copy of the typed slices so "Other" aggregation
     * doesn't mutate the persistent self->slices state. The temporary
     * doesn't own the label pointers; aggregation uses the
     * persistent copy's strings and the optional self->pie_other_label
     * for the Other bucket, so no per-slot ownership churn. */
    fastchart_pie_slice slices[FASTCHART_MAX_SLICES];
    int n_slices = self->slice_count;
    double total = self->total;
    for (int i = 0; i < n_slices; i++) slices[i] = self->slices[i];

    if (self->pie_other_threshold > 0.0 && total > 0.0 && n_slices > 1) {
        double threshold_value = total * (self->pie_other_threshold / 100.0);
        fastchart_pie_slice kept[FASTCHART_MAX_SLICES];
        int n_kept = 0;
        double other_sum = 0.0;
        int other_count = 0;
        for (int i = 0; i < n_slices; i++) {
            if (slices[i].value < threshold_value) {
                other_sum += slices[i].value;
                other_count++;
            } else if (n_kept < FASTCHART_MAX_SLICES) {
                kept[n_kept++] = slices[i];
            }
        }
        if (other_count > 0 && n_kept < FASTCHART_MAX_SLICES) {
            kept[n_kept].label = self->pie_other_label
                ? ZSTR_VAL(self->pie_other_label) : (char *)"Other";
            kept[n_kept].idx_label[0] = '\0';
            kept[n_kept].value = other_sum;
            kept[n_kept].color_rgb = -1;
            n_kept++;
        }
        for (int i = 0; i < n_kept; i++) slices[i] = kept[i];
        n_slices = n_kept;
    }

    double donut = self->donut_hole_ratio;
    if (donut < 0) donut = 0;
    if (donut >= 1.0) donut = 0.95;

    fastchart_rect plot;
    /* No axes for pie charts -- pass 0/0 so layout reserves space
     * only for the title. */
    fastchart_compute_layout((fastchart_obj *)self, im, 0, 0, &plot);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, (fastchart_obj *)self, &pal);

    fastchart_draw_frame(im, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_title(im, (fastchart_obj *)self, &plot, &pal);

    /* Pie geometry: largest disk that fits, centered in the plot
     * rect, with a margin reserved for label leaders / outside text. */
    int cx = (plot.x0 + plot.x1) / 2;
    int cy = (plot.y0 + plot.y1) / 2;
    int avail_w = (plot.x1 - plot.x0);
    int avail_h = (plot.y1 - plot.y0);
    int diameter = (avail_w < avail_h ? avail_w : avail_h) - 60;
    if (diameter < 40) diameter = 40;

    /* Per-slice radial offset from setExplode([idx => px, ...]),
     * pre-parsed at setter time into self->explode. */
    const zend_long *explode = self->explode;
    int explode_count = self->explode_count;

    /* Slices via gdImageFilledArc. gdPie produces a filled wedge;
     * gdNoFill + gdEdged outlines without filling. We draw the wedge
     * with gdPie + a thin outline pass for slice separation.
     *
     * Resolve every slice's color once before the draw loop so the
     * loop body reads from a flat int[] instead of calling
     * gdImageColorAllocate per slice. */
    int *slice_colors = ecalloc((size_t)n_slices, sizeof(int));
    for (int i = 0; i < n_slices; i++) {
        if (slices[i].color_rgb >= 0) {
            slice_colors[i] = gdImageColorAllocate(im,
                (slices[i].color_rgb >> 16) & 0xFF,
                (slices[i].color_rgb >>  8) & 0xFF,
                 slices[i].color_rgb        & 0xFF);
        } else {
            slice_colors[i] = pal.series[i % FASTCHART_PALETTE_SERIES_N];
        }
    }

    double start_deg = -90.0;  /* 12 o'clock */
    for (int i = 0; i < n_slices; i++) {
        double sweep = 360.0 * (slices[i].value / total);
        int color = slice_colors[i];

        /* Explode this slice radially outward by `offset` pixels
         * along its mid-angle. Slices not mentioned stay at center. */
        int slice_cx = cx, slice_cy = cy;
        if (explode && i < explode_count) {
            zend_long off = explode[i];
            if (off > 0) {
                double mid_rad = (start_deg + sweep / 2.0) * M_PI / 180.0;
                slice_cx = cx + (int)((double)off * cos(mid_rad));
                slice_cy = cy + (int)((double)off * sin(mid_rad));
            }
        }

        fastchart_shadow_filled_arc(im, (fastchart_obj *)self, slice_cx, slice_cy, diameter,
                                    (int)floor(start_deg),
                                    (int)ceil(start_deg + sweep));
        gdImageFilledArc(im, slice_cx, slice_cy, diameter, diameter,
                         (int)floor(start_deg), (int)ceil(start_deg + sweep),
                         color, gdPie);
        int edge = self->edge_color >= 0 ? (int)self->edge_color : pal.border;
        gdImageFilledArc(im, slice_cx, slice_cy, diameter, diameter,
                         (int)floor(start_deg), (int)ceil(start_deg + sweep),
                         edge, gdNoFill | gdEdged);
        start_deg += sweep;
    }

    /* Donut overdraw: paint a plot-bg-colored disk over the center.
     * Note: with explode, the donut hole stays centered (which is
     * the standard appearance -- exploding slices leave the center
     * looking ring-broken on purpose). */
    if (donut > 0) {
        int hole = (int)((double)diameter * donut);
        if (hole < 4) hole = 4;
        gdImageFilledEllipse(im, cx, cy, hole, hole, pal.plot_bg);
    }

    /* Slice labels: percentage at the slice's mid-angle. Position
     * mode picks INSIDE (default), OUTSIDE (with leader line), or
     * NONE. Format string defaults to "%.0f%%"; the user-supplied
     * format receives the percentage as the sole sprintf argument. */
    const char *font = (self->slice_label_position != FASTCHART_LABEL_NONE)
        ? fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL) : NULL;
    if (font) {
        const char *fmt = self->slice_label_format
            ? ZSTR_VAL(self->slice_label_format)
            : "%.0f%%";
        double size = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;

        double inside_frac = donut > 0 ? (donut + 1.0) / 2.0 : 0.7;
        double inside_r = (diameter / 2.0) * inside_frac;
        double outside_r = (diameter / 2.0) + 14.0;

        start_deg = -90.0;
        for (int i = 0; i < n_slices; i++) {
            double sweep = 360.0 * (slices[i].value / total);
            if (sweep < 4.0) { start_deg += sweep; continue; }

            double mid_rad = (start_deg + sweep / 2.0) * M_PI / 180.0;
            double cos_mid = cos(mid_rad);
            double sin_mid = sin(mid_rad);
            char buf[64];
            snprintf(buf, sizeof(buf), fmt, 100.0 * slices[i].value / total);

            if (self->slice_label_position == FASTCHART_LABEL_LEFT ||
                self->slice_label_position == FASTCHART_LABEL_RIGHT) {
                /* Force all labels to one side with leader lines from
                 * the slice rim. Useful when many small slices crowd
                 * a particular sector and OUTSIDE-mode labels overlap. */
                bool right_side = (self->slice_label_position == FASTCHART_LABEL_RIGHT);
                int lx = right_side
                    ? cx + (int)((diameter / 2.0) + 14.0)
                    : cx - (int)((diameter / 2.0) + 14.0);
                int ly = cy + (int)(outside_r * sin_mid);
                int rim_x = cx + (int)((diameter / 2.0) * cos_mid);
                int rim_y = cy + (int)((diameter / 2.0) * sin_mid);
                gdImageLine(im, rim_x, rim_y, lx, ly, pal.axis);
                fastchart_align align = right_side
                    ? FASTCHART_ALIGN_LEFT : FASTCHART_ALIGN_RIGHT;
                int anchor_x = lx + (right_side ? 4 : -4);
                fastchart_text_draw(im, font, size, pal.text,
                                    anchor_x, ly + (int)(size * 0.35),
                                    align, buf, NULL, 0);
            } else if (self->slice_label_position == FASTCHART_LABEL_OUTSIDE) {
                int lx = cx + (int)(outside_r * cos_mid);
                int ly = cy + (int)(outside_r * sin_mid);
                /* Tiny leader line from rim to label anchor. */
                int rim_x = cx + (int)((diameter / 2.0) * cos_mid);
                int rim_y = cy + (int)((diameter / 2.0) * sin_mid);
                gdImageLine(im, rim_x, rim_y, lx, ly, pal.axis);
                bool right_side = (cos_mid >= 0);
                fastchart_align align = right_side
                    ? FASTCHART_ALIGN_LEFT : FASTCHART_ALIGN_RIGHT;
                int anchor_x = lx + (right_side ? 4 : -4);
                fastchart_text_draw(im, font, size, pal.text,
                                    anchor_x, ly + (int)(size * 0.35),
                                    align, buf, NULL, 0);
            } else {
                /* INSIDE -- skip labels for very small slices to
                 * avoid overlap. The 8-degree threshold roughly
                 * matches the GDChart default. */
                if (sweep >= 8.0) {
                    int lx = cx + (int)(inside_r * cos_mid);
                    int ly = cy + (int)(inside_r * sin_mid);
                    fastchart_text_draw(im, font, size, pal.text,
                                        lx, ly + (int)(size * 0.35),
                                        FASTCHART_ALIGN_CENTER, buf, NULL, 0);
                }
            }
            start_deg += sweep;
        }
    }

    fastchart_draw_text_annotations(im, (fastchart_obj *)self, &pal);
    efree(slice_colors);
    return 0;
}

ZEND_METHOD(FastChart_PieChart, draw)
{
    zval *canvas_zv;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\PieChart::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }

    fastchart_pie_obj *self = Z_FASTCHART_PIE_OBJ_P(ZEND_THIS);
    if (fastchart_pie_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
