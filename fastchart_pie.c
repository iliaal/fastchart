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

#define MAX_SLICES 32

typedef struct {
    const char *label;     /* may be NULL */
    char        idx_label[16];  /* fallback "0" / "1" / etc. */
    double      value;     /* > 0 only -- nonpositive slices skipped */
    int         color_rgb; /* explicit color override; -1 means use palette */
} fastchart_pie_slice;

/* Accept two shapes for the slice array:
 *   1. ['label' => value, 'label2' => value, ...]  (associative map)
 *   2. [['label' => 'X', 'value' => 1.5], ...]     (list of dicts)
 *
 * Detection: if every key is a string and every value is numeric,
 * it's shape 1. Otherwise shape 2 (drop entries that don't conform). */

static int collect_pie_slices(zval *data_zv,
                              fastchart_pie_slice *out, int max_slices,
                              int *out_count, double *out_total)
{
    *out_count = 0;
    *out_total = 0;
    if (Z_TYPE_P(data_zv) != IS_ARRAY) return -1;
    HashTable *ht = Z_ARRVAL_P(data_zv);
    int n = (int)zend_hash_num_elements(ht);
    if (n == 0) return 0;

    /* Decide shape by sampling first element. */
    int shape_assoc = 1;
    {
        zend_string *k;
        zend_ulong h;
        zval *v;
        ZEND_HASH_FOREACH_KEY_VAL(ht, h, k, v) {
            (void)h;
            if (!k || (Z_TYPE_P(v) != IS_LONG && Z_TYPE_P(v) != IS_DOUBLE)) {
                shape_assoc = 0;
            }
            break;
        } ZEND_HASH_FOREACH_END();
    }

    if (shape_assoc) {
        zend_string *k;
        zend_ulong h;
        zval *v;
        ZEND_HASH_FOREACH_KEY_VAL(ht, h, k, v) {
            (void)h;
            if (*out_count >= max_slices) break;
            double d;
            if (fastchart_zval_to_double(v, &d) != 0) continue;
            if (d <= 0.0 || !isfinite(d)) continue;
            out[*out_count].label = k ? ZSTR_VAL(k) : NULL;
            if (!k) {
                snprintf(out[*out_count].idx_label,
                         sizeof(out[*out_count].idx_label),
                         "%d", *out_count);
            } else {
                out[*out_count].idx_label[0] = '\0';
            }
            out[*out_count].value = d;
            out[*out_count].color_rgb = -1;
            *out_total += d;
            (*out_count)++;
        } ZEND_HASH_FOREACH_END();
    } else {
        zval *entry;
        ZEND_HASH_FOREACH_VAL(ht, entry) {
            if (Z_TYPE_P(entry) != IS_ARRAY) continue;
            if (*out_count >= max_slices) break;

            zval *label_zv = zend_hash_str_find(Z_ARRVAL_P(entry),
                                                "label", sizeof("label") - 1);
            zval *value_zv = zend_hash_str_find(Z_ARRVAL_P(entry),
                                                "value", sizeof("value") - 1);
            if (!value_zv) continue;

            double d;
            if (fastchart_zval_to_double(value_zv, &d) != 0) continue;
            if (d <= 0.0 || !isfinite(d)) continue;

            if (label_zv && Z_TYPE_P(label_zv) == IS_STRING) {
                out[*out_count].label = Z_STRVAL_P(label_zv);
                out[*out_count].idx_label[0] = '\0';
            } else {
                out[*out_count].label = NULL;
                snprintf(out[*out_count].idx_label,
                         sizeof(out[*out_count].idx_label),
                         "%d", *out_count);
            }
            out[*out_count].value = d;

            /* Optional 'color' key. Accept long (24-bit packed
             * 0xRRGGBB). Out-of-range values fall back to palette
             * rather than error -- this mirrors how callers
             * typically generate colors (often via a hash with no
             * a-priori bounds check). */
            out[*out_count].color_rgb = -1;
            zval *color_zv = zend_hash_str_find(Z_ARRVAL_P(entry),
                                                "color", sizeof("color") - 1);
            if (color_zv && Z_TYPE_P(color_zv) == IS_LONG) {
                zend_long c = Z_LVAL_P(color_zv);
                if (c >= 0 && c <= 0xFFFFFF) {
                    out[*out_count].color_rgb = (int)c;
                }
            }

            *out_total += d;
            (*out_count)++;
        } ZEND_HASH_FOREACH_END();
    }

    return 0;
}

int fastchart_pie_render_to_image(fastchart_obj *self, gdImagePtr im)
{
    fastchart_pie_slice slices[MAX_SLICES];
    int n_slices = 0;
    double total = 0;
    if (collect_pie_slices(&self->data, slices, MAX_SLICES,
                           &n_slices, &total) != 0 || n_slices == 0) {
        zend_throw_error(NULL,
            "FastChart\\PieChart::draw() requires setSlices() with one or more positive values");
        return -1;
    }

    /* "Other" aggregation: collapse slices below threshold% into a
     * single Other slice. We rebuild the slice list in place,
     * keeping the ones above threshold and accumulating the rest. */
    if (self->pie_other_threshold > 0.0 && total > 0.0 && n_slices > 1) {
        double threshold_value = total * (self->pie_other_threshold / 100.0);
        fastchart_pie_slice kept[MAX_SLICES];
        int n_kept = 0;
        double other_sum = 0.0;
        int other_count = 0;
        for (int i = 0; i < n_slices; i++) {
            if (slices[i].value < threshold_value) {
                other_sum += slices[i].value;
                other_count++;
            } else if (n_kept < MAX_SLICES) {
                kept[n_kept++] = slices[i];
            }
        }
        if (other_count > 0 && n_kept < MAX_SLICES) {
            const char *other_label = self->pie_other_label
                ? ZSTR_VAL(self->pie_other_label) : "Other";
            kept[n_kept].label = other_label;
            kept[n_kept].idx_label[0] = '\0';
            kept[n_kept].value = other_sum;
            kept[n_kept].color_rgb = -1;  /* palette default */
            n_kept++;
        }
        for (int i = 0; i < n_kept; i++) slices[i] = kept[i];
        n_slices = n_kept;
        /* total unchanged: aggregating doesn't change the sum */
    }

    double donut = 0.0;
    {
        zval *cfg = zend_hash_str_find(Z_ARRVAL(self->config),
                                       "donut_hole_ratio",
                                       sizeof("donut_hole_ratio") - 1);
        if (cfg) fastchart_zval_to_double(cfg, &donut);
    }
    if (donut < 0) donut = 0;
    if (donut >= 1.0) donut = 0.95;

    fastchart_rect plot;
    /* No axes for pie charts -- pass 0/0 so layout reserves space
     * only for the title. */
    fastchart_compute_layout(self, im, 0, 0, &plot);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, self, &pal);

    fastchart_draw_frame(im, self, &plot, &pal);
    fastchart_draw_title(im, self, &plot, &pal);

    /* Pie geometry: largest disk that fits, centered in the plot
     * rect, with a margin reserved for label leaders / outside text. */
    int cx = (plot.x0 + plot.x1) / 2;
    int cy = (plot.y0 + plot.y1) / 2;
    int avail_w = (plot.x1 - plot.x0);
    int avail_h = (plot.y1 - plot.y0);
    int diameter = (avail_w < avail_h ? avail_w : avail_h) - 60;
    if (diameter < 40) diameter = 40;

    /* Per-slice radial offset from setExplode([idx => px, ...]). */
    zval *explode_zv = zend_hash_str_find(Z_ARRVAL(self->config),
                                          "explode", sizeof("explode") - 1);
    HashTable *explode_ht =
        (explode_zv && Z_TYPE_P(explode_zv) == IS_ARRAY) ? Z_ARRVAL_P(explode_zv) : NULL;

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
        if (explode_ht) {
            zval *off_zv = zend_hash_index_find(explode_ht, i);
            if (off_zv) {
                zend_long off = 0;
                if (Z_TYPE_P(off_zv) == IS_LONG) off = Z_LVAL_P(off_zv);
                else if (Z_TYPE_P(off_zv) == IS_DOUBLE) off = (long)Z_DVAL_P(off_zv);
                if (off > 0) {
                    double mid_rad = (start_deg + sweep / 2.0) * M_PI / 180.0;
                    slice_cx = cx + (int)((double)off * cos(mid_rad));
                    slice_cy = cy + (int)((double)off * sin(mid_rad));
                }
            }
        }

        fastchart_shadow_filled_arc(im, self, slice_cx, slice_cy, diameter,
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
        ? fastchart_resolve_font(self, FC_FONT_LABEL) : NULL;
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
                int ly = cy + (int)(outside_r * sin(mid_rad));
                int rim_x = cx + (int)((diameter / 2.0) * cos(mid_rad));
                int rim_y = cy + (int)((diameter / 2.0) * sin(mid_rad));
                gdImageLine(im, rim_x, rim_y, lx, ly, pal.axis);
                fastchart_align align = right_side
                    ? FASTCHART_ALIGN_LEFT : FASTCHART_ALIGN_RIGHT;
                int anchor_x = lx + (right_side ? 4 : -4);
                fastchart_text_draw(im, font, size, pal.text,
                                    anchor_x, ly + (int)(size * 0.35),
                                    align, buf, NULL, 0);
            } else if (self->slice_label_position == FASTCHART_LABEL_OUTSIDE) {
                int lx = cx + (int)(outside_r * cos(mid_rad));
                int ly = cy + (int)(outside_r * sin(mid_rad));
                /* Tiny leader line from rim to label anchor. */
                int rim_x = cx + (int)((diameter / 2.0) * cos(mid_rad));
                int rim_y = cy + (int)((diameter / 2.0) * sin(mid_rad));
                gdImageLine(im, rim_x, rim_y, lx, ly, pal.axis);
                fastchart_align align = (cos(mid_rad) >= 0)
                    ? FASTCHART_ALIGN_LEFT : FASTCHART_ALIGN_RIGHT;
                int anchor_x = lx + (cos(mid_rad) >= 0 ? 4 : -4);
                fastchart_text_draw(im, font, size, pal.text,
                                    anchor_x, ly + (int)(size * 0.35),
                                    align, buf, NULL, 0);
            } else {
                /* INSIDE -- skip labels for very small slices to
                 * avoid overlap. The 8-degree threshold roughly
                 * matches the GDChart default. */
                if (sweep >= 8.0) {
                    int lx = cx + (int)(inside_r * cos(mid_rad));
                    int ly = cy + (int)(inside_r * sin(mid_rad));
                    fastchart_text_draw(im, font, size, pal.text,
                                        lx, ly + (int)(size * 0.35),
                                        FASTCHART_ALIGN_CENTER, buf, NULL, 0);
                }
            }
            start_deg += sweep;
        }
    }

    fastchart_draw_text_annotations(im, self, &pal);
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

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (fastchart_pie_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
