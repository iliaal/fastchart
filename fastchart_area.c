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

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_target.h"
#include "fastchart_axis.h"
#include "fastchart_effects.h"

/* Read a value from a typed series at the given index. Returns NaN
 * if the index is past the series end or the cell is a gap. */
static inline double area_read_value(const fastchart_series_t *s, int i)
{
    if (i >= s->len) return NAN;
    return s->values[i];
}

int fastchart_area_render_to_image(fastchart_area_obj *self, gdImagePtr im)
{
    fastchart_target_t t;
    fastchart_target_from_gd(&t, im, self->dpi);
    if (self->n_series == 0) {
        zend_throw_error(NULL,
            "FastChart\\AreaChart::draw() requires setSeries() to have been called with non-empty data");
        return -1;
    }
    fastchart_series_t *series = self->series;
    int n_series = self->n_series;
    int max_len = self->max_len;

    bool stacked = self->stacked;
    if (n_series < 2) stacked = false;

    double dmin = 0, dmax = 0;
    int seen = 0;

    if (stacked) {
        for (int i = 0; i < max_len; i++) {
            double sum = 0;
            for (int s = 0; s < n_series; s++) {
                double v = area_read_value(&series[s], i);
                if (!isnan(v)) sum += v;
            }
            if (!seen) { dmin = 0; dmax = sum; seen = 1; }
            else if (sum > dmax) dmax = sum;
        }
    } else {
        for (int s = 0; s < n_series; s++) {
            for (int i = 0; i < series[s].len; i++) {
                double d = series[s].values[i];
                if (isnan(d)) continue;
                if (!seen) { dmin = dmax = d; seen = 1; }
                else { if (d < dmin) dmin = d; if (d > dmax) dmax = d; }
            }
        }
        if (dmin > 0) dmin = 0;
    }
    if (!seen) {
        zend_throw_error(NULL,
            "FastChart\\AreaChart::draw() found no numeric values");
        return -1;
    }

    fastchart_value_range range;
    if (self->y_axis_scale == FASTCHART_SCALE_LOG) {
        if (dmin <= 0) {
            zend_value_error("FastChart\\AreaChart::draw(): log Y-axis requires strictly-positive data");
            return -1;
        }
        if (fastchart_value_range_compute_log(dmin, dmax, &range) != 0) {
            zend_value_error("FastChart\\AreaChart::draw(): log Y-axis requires strictly-positive data");
            return -1;
        }
    } else {
        fastchart_value_range_compute(dmin, dmax, 6, &range);
        fastchart_value_range_apply_override((fastchart_obj *)self, &range);
    }

    fastchart_rect plot;
    fastchart_compute_layout((fastchart_obj *)self, &t, 1, 1, NULL, 0, &plot);

    fastchart_palette pal;
    fastchart_palette_init(&t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(&t, (fastchart_obj *)self, &pal);

    fastchart_gradient_cache grad_cache;
    fastchart_gradient_cache_reset(&grad_cache);

    fastchart_draw_frame(&t, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_title(&t, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_y_axis(&t, (fastchart_obj *)self, &plot, &pal, &range);
    fastchart_draw_plot_bands(&t, (fastchart_obj *)self, &plot, &range, &pal);
    fastchart_draw_v_plot_bands_categorical(&t, (fastchart_obj *)self, &plot,
                                            max_len, &pal);

    const char **label_ptrs = fastchart_borrow_category_labels((fastchart_obj *)self, max_len);
    fastchart_draw_x_axis_categorical(&t, (fastchart_obj *)self, &plot, &pal, max_len, label_ptrs);
    if (label_ptrs) efree((void *)label_ptrs);

    fastchart_draw_axis_titles(&t, (fastchart_obj *)self, &plot, &pal);

    int alpha = (int)self->area_alpha;
    if (alpha < 0) alpha = 0;
    if (alpha > 127) alpha = 127;

    /* Build filled polygons. For stacked, accumulate per-category
     * sums; each series's polygon spans [prev_cum, prev_cum + v].
     * For non-stacked overlay, each series's polygon spans
     * [0, v] with translucent fill so layered shapes show through. */
    gdPoint poly[2 * FASTCHART_MAX_POINTS_PER_SERIES];

    if (stacked) {
        double *cum = ecalloc((size_t)max_len, sizeof(double));
        for (int s = 0; s < n_series; s++) {
            /* pal.series[] holds target handles; resolve to a gd-int
             * for the direct effects.h primitives below. */
            int rgb_color = fastchart_target_color_to_gd(&t,
                pal.series[s % FASTCHART_PALETTE_SERIES_N]);
            int n_pts = 0;

            /* Top edge: left to right at cum + v. */
            for (int i = 0; i < max_len && n_pts < 2048; i++) {
                double v = area_read_value(&series[s], i);
                if (isnan(v)) v = 0;
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                int y = fastchart_y_to_pixel(cum[i] + v, &range, &plot);
                poly[n_pts].x = x; poly[n_pts].y = y;
                n_pts++;
            }
            /* Bottom edge: right to left at cum. */
            for (int i = max_len - 1; i >= 0 && n_pts < 2 * 2048; i--) {
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                int y = fastchart_y_to_pixel(cum[i], &range, &plot);
                poly[n_pts].x = x; poly[n_pts].y = y;
                n_pts++;
            }
            if (n_pts >= 3) {
                fastchart_shadow_filled_polygon(im, (fastchart_obj *)self, poly, n_pts);
                if (!fastchart_gradient_filled_polygon(im, (fastchart_obj *)self, &grad_cache, poly, n_pts)) {
                    fastchart_filled_polygon_aa(im, poly, n_pts, rgb_color);
                }
                if (self->edge_color >= 0) {
                    gdImagePolygon(im, poly, n_pts, (int)self->edge_color);
                }
            }

            /* Top-edge stroke for crisp boundary between layers. */
            int prev_x = 0, prev_y = 0;
            bool prev_valid = false;
            for (int i = 0; i < max_len; i++) {
                double v = area_read_value(&series[s], i);
                if (isnan(v)) v = 0;
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                int y = fastchart_y_to_pixel(cum[i] + v, &range, &plot);
                if (prev_valid) {
                    gdImageLine(im, prev_x, prev_y, x, y, fastchart_target_color_to_gd(&t, pal.border));
                }
                prev_x = x; prev_y = y; prev_valid = true;
                cum[i] += v;
            }
        }
        efree(cum);
    } else {
        int zero_y = fastchart_y_to_pixel(dmin > 0 ? dmin : 0.0, &range, &plot);

        for (int s = 0; s < n_series; s++) {
            /* pal.series[] is a target handle; translate to gd-int so
             * the alpha-blended fill and AA outline below can address
             * libgd directly. */
            int base_color = fastchart_target_color_to_gd(&t,
                pal.series[s % FASTCHART_PALETTE_SERIES_N]);
            /* gdImageRed/Green/Blue work on both palette and truecolor
             * canvases; bit-shifting `base_color` would only work on
             * truecolor (where the handle is the packed RGB). */
            int r = gdImageRed(im, base_color);
            int g = gdImageGreen(im, base_color);
            int b = gdImageBlue(im, base_color);
            int alpha_color = gdImageColorAllocateAlpha(im, r, g, b, alpha);

            int n_pts = 0;
            for (int i = 0; i < max_len && n_pts < 2048; i++) {
                double v = area_read_value(&series[s], i);
                if (isnan(v)) v = 0;
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                int y = fastchart_y_to_pixel(v, &range, &plot);
                poly[n_pts].x = x; poly[n_pts].y = y;
                n_pts++;
            }
            for (int i = max_len - 1; i >= 0 && n_pts < 2 * 2048; i--) {
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                poly[n_pts].x = x; poly[n_pts].y = zero_y;
                n_pts++;
            }
            if (n_pts >= 3) {
                fastchart_shadow_filled_polygon(im, (fastchart_obj *)self, poly, n_pts);
                gdImageAlphaBlending(im, 1);
                if (!fastchart_gradient_filled_polygon(im, (fastchart_obj *)self, &grad_cache, poly, n_pts)) {
                    fastchart_filled_polygon_aa(im, poly, n_pts, alpha_color);
                }
                if (self->edge_color >= 0) {
                    gdImagePolygon(im, poly, n_pts, (int)self->edge_color);
                }
                gdImageAlphaBlending(im, 0);
            }

            /* Opaque top stroke. */
            gdImageSetThickness(im, 2);
            gdImageSetAntiAliased(im, base_color);
            int prev_x = 0, prev_y = 0;
            bool prev_valid = false;
            for (int i = 0; i < max_len; i++) {
                double v = area_read_value(&series[s], i);
                if (isnan(v)) { prev_valid = false; continue; }
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                int y = fastchart_y_to_pixel(v, &range, &plot);
                if (prev_valid) gdImageLine(im, prev_x, prev_y, x, y, gdAntiAliased);
                prev_x = x; prev_y = y; prev_valid = true;
            }
            gdImageSetThickness(im, 1);
        }
    }

    /* Combo overlays + annotations on top of the area fills. */
    fastchart_draw_overlays_categorical(&t, (fastchart_obj *)self, &plot, &pal,
                                         &range, NULL, max_len);

    fastchart_draw_h_annotations(&t, (fastchart_obj *)self, &plot, &pal, &range);
    fastchart_draw_v_annotations_categorical(&t, (fastchart_obj *)self, &plot, &pal, max_len);

    /* Legend. */
    if (n_series >= 2) {
        int legend_colors[FASTCHART_MAX_SERIES];
        const char *legend_labels[FASTCHART_MAX_SERIES];
        int legend_count = 0;
        for (int s = 0; s < n_series; s++) {
            if (!series[s].label) continue;
            legend_colors[legend_count] = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            legend_labels[legend_count] = series[s].label;
            legend_count++;
        }
        if (legend_count > 0) {
            fastchart_draw_legend(&t, (fastchart_obj *)self, &plot, &pal,
                                  legend_count, legend_colors, legend_labels);
        }
    }

    fastchart_draw_text_annotations(&t, (fastchart_obj *)self, &pal);

    if (self->icons && self->n_icons > 0 && max_len > 0) {
        for (int i = 0; i < self->n_icons; i++) {
            const fastchart_icon *ic = &self->icons[i];
            double frac_x = max_len > 1
                ? (ic->x + 0.5) / (double)max_len
                : 0.5;
            int px = plot.x0 + (int)(frac_x * (plot.x1 - plot.x0) + 0.5);
            int py = fastchart_y_to_pixel(ic->y, &range, &plot);
            fastchart_blit_icon(&t, ic, px, py);
        }
    }
    return 0;
}

ZEND_METHOD(FastChart_AreaChart, draw)
{
    zval *canvas_zv;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\AreaChart::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }
    if (!fastchart_require_truecolor(im)) RETURN_THROWS();

    fastchart_area_obj *self = Z_FASTCHART_AREA_OBJ_P(ZEND_THIS);
    if (fastchart_area_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
