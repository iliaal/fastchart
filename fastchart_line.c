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

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_axis.h"

int fastchart_line_render_to_image(fastchart_line_obj *self, gdImagePtr im)
{
    if (self->n_series == 0) {
        zend_throw_error(NULL,
            "FastChart\\LineChart::draw() requires setSeries() to have been called with non-empty data");
        return -1;
    }
    fastchart_series_t *series = self->series;
    int n_series = self->n_series;
    int max_len = self->max_len;

    /* Compute left + right Y ranges from typed values[]. With
     * secondary_y off, all series go to the left axis regardless of
     * the per-series right_axis flag. */
    int n_right = 0;
    if (self->secondary_y) {
        for (int s = 0; s < n_series; s++) {
            if (series[s].right_axis) n_right++;
        }
    }

    double dmin_l = 0, dmax_l = 0, dmin_r = 0, dmax_r = 0;
    int seen_l = 0, seen_r = 0;
    for (int s = 0; s < n_series; s++) {
        bool right = self->secondary_y && series[s].right_axis;
        for (int i = 0; i < series[s].len; i++) {
            double d = series[s].values[i];
            if (isnan(d)) continue;
            if (right) {
                if (!seen_r) { dmin_r = dmax_r = d; seen_r = 1; }
                else { if (d < dmin_r) dmin_r = d; if (d > dmax_r) dmax_r = d; }
            } else {
                if (!seen_l) { dmin_l = dmax_l = d; seen_l = 1; }
                else { if (d < dmin_l) dmin_l = d; if (d > dmax_l) dmax_l = d; }
            }
        }
    }
    if (!seen_l) {
        zend_throw_error(NULL,
            "FastChart\\LineChart::draw() found no numeric values for the primary Y axis");
        return -1;
    }
    if (n_right > 0 && !seen_r) {
        zend_throw_error(NULL,
            "FastChart\\LineChart::draw() found no numeric values for the secondary Y axis");
        return -1;
    }

    fastchart_value_range range_l, range_r;
    if (self->y_axis_scale == FASTCHART_SCALE_LOG) {
        if (fastchart_value_range_compute_log(dmin_l, dmax_l, &range_l) != 0) {
            zend_value_error("FastChart\\LineChart::draw(): log Y-axis requires strictly-positive data");
            return -1;
        }
    } else {
        fastchart_value_range_compute(dmin_l, dmax_l, 6, &range_l);
        fastchart_value_range_apply_override((fastchart_obj *)self, &range_l);
    }
    if (n_right > 0) {
        fastchart_value_range_compute(dmin_r, dmax_r, 6, &range_r);
        fastchart_value_range_apply_override((fastchart_obj *)self, &range_r);
    }

    fastchart_rect plot;
    fastchart_compute_layout((fastchart_obj *)self, im, 1, 1, NULL, 0, &plot);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, (fastchart_obj *)self, &pal);

    fastchart_draw_frame(im, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_title(im, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_y_axis(im, (fastchart_obj *)self, &plot, &pal, &range_l);
    if (n_right > 0) {
        fastchart_draw_y_axis_right(im, (fastchart_obj *)self, &plot, &pal, &range_r);
    }
    fastchart_draw_plot_bands(im, (fastchart_obj *)self, &plot, &range_l, &pal);
    fastchart_draw_v_plot_bands_categorical(im, (fastchart_obj *)self, &plot,
                                            max_len, &pal);

    const char **label_ptrs = fastchart_borrow_category_labels((fastchart_obj *)self, max_len);
    fastchart_draw_x_axis_categorical(im, (fastchart_obj *)self, &plot, &pal, max_len, label_ptrs);
    if (label_ptrs) efree((void *)label_ptrs);

    fastchart_draw_axis_titles(im, (fastchart_obj *)self, &plot, &pal);

    int marker_style = self->marker_style >= 0
        ? (int)self->marker_style
        : FASTCHART_MARKER_CIRCLE;
    int marker_size = self->marker_size >= 1
        ? (int)self->marker_size
        : 6;

    int legend_colors[FASTCHART_MAX_SERIES];
    const char *legend_labels[FASTCHART_MAX_SERIES];
    int legend_count = 0;

    /* Optional per-point error bars (parallel to the first series). */
    double *err_lo = self->err_lo;
    double *err_hi = self->err_hi;
    int err_n = self->err_n;

    fastchart_pt pts[FASTCHART_MAX_POINTS_PER_SERIES];
    for (int s = 0; s < n_series; s++) {
        int color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
        bool right = self->secondary_y && series[s].right_axis;
        const fastchart_value_range *rng = right ? &range_r : &range_l;
        double *values = series[s].values;
        /* setSeries rejects > FASTCHART_MAX_POINTS_PER_SERIES, so the
         * clamp is defensive — the parser already capped this. */
        int n = series[s].len;
        if (n < 1) continue;

        /* Build the polyline points (NaN -> invalid -> gap). */
        for (int i = 0; i < n; i++) {
            pts[i].x = fastchart_x_categorical_center(&plot, i, max_len);
            if (isnan(values[i])) {
                pts[i].valid = false;
            } else {
                pts[i].y = fastchart_y_to_pixel(values[i], rng, &plot);
                pts[i].valid = true;
            }
        }

        /* Error bars on the first (left-axis) series only -- multi-
         * series line charts get crowded fast otherwise. */
        if (err_lo && err_n > 0 && s == 0 && !right) {
            int lim = n < err_n ? n : err_n;
            for (int i = 0; i < lim; i++) {
                if (!pts[i].valid) continue;
                double lo = err_lo[i];
                double hi = err_hi[i];
                if (isnan(lo) && isnan(hi)) continue;
                if (isnan(lo)) lo = 0;
                if (isnan(hi)) hi = 0;
                if (lo > 0 || hi > 0) {
                    int py_lo = fastchart_y_to_pixel(values[i] - lo, rng, &plot);
                    int py_hi = fastchart_y_to_pixel(values[i] + hi, rng, &plot);
                    int x = pts[i].x;
                    gdImageLine(im, x, py_hi, x, py_lo, pal.axis);
                    gdImageLine(im, x - 4, py_hi, x + 4, py_hi, pal.axis);
                    gdImageLine(im, x - 4, py_lo, x + 4, py_lo, pal.axis);
                }
            }
        }

        fastchart_draw_polyline(im, (fastchart_obj *)self, pts, n, color, 2, true);

        for (int i = 0; i < n; i++) {
            if (!pts[i].valid) continue;
            int marker_color = color;
            if (series[s].point_colors) {
                zend_long c = series[s].point_colors[i];
                if (c >= 0) {
                    marker_color = gdImageColorAllocate(im,
                        (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
                }
            }
            fastchart_draw_marker(im, pts[i].x, pts[i].y,
                                  marker_style, marker_size, marker_color);
            fastchart_draw_value_label(im, (fastchart_obj *)self, &pal,
                                       pts[i].x, pts[i].y, values[i]);
        }

        if (series[s].label) {
            legend_colors[legend_count] = color;
            legend_labels[legend_count] = series[s].label;
            legend_count++;
        }
    }

    /* Combo overlays go on top of the primary data. */
    fastchart_draw_overlays_categorical(im, (fastchart_obj *)self, &plot, &pal,
                                         &range_l,
                                         n_right > 0 ? &range_r : NULL,
                                         max_len);

    fastchart_draw_h_annotations(im, (fastchart_obj *)self, &plot, &pal, &range_l);
    fastchart_draw_v_annotations_categorical(im, (fastchart_obj *)self, &plot, &pal, max_len);

    if (legend_count >= 1 && n_series >= 2) {
        fastchart_draw_legend(im, (fastchart_obj *)self, &plot, &pal,
                              legend_count, legend_colors, legend_labels);
    }

    fastchart_draw_text_annotations(im, (fastchart_obj *)self, &pal);

    /* IconPlot overlays at data coordinates. x is treated as a
     * fractional category index (0 = first category, max_len-1 = last);
     * out-of-range values still render but get clipped to the plot
     * edges. y is data-y on the left axis. */
    if (self->icons && self->n_icons > 0 && max_len > 0) {
        for (int i = 0; i < self->n_icons; i++) {
            const fastchart_icon *ic = &self->icons[i];
            double frac_x = max_len > 1
                ? (ic->x + 0.5) / (double)max_len
                : 0.5;
            int px = plot.x0 + (int)(frac_x * (plot.x1 - plot.x0) + 0.5);
            int py = fastchart_y_to_pixel(ic->y, &range_l, &plot);
            fastchart_blit_icon(im, ic, px, py);
        }
    }

    return 0;
}

ZEND_METHOD(FastChart_LineChart, draw)
{
    zval *canvas_zv;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\LineChart::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }
    if (!fastchart_require_truecolor(im)) RETURN_THROWS();

    fastchart_line_obj *self = Z_FASTCHART_LINE_OBJ_P(ZEND_THIS);
    if (fastchart_line_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
