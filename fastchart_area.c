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

/* Read a value from a typed series at the given index. Returns NaN
 * if the index is past the series end or the cell is a gap. */
static inline double area_read_value(const fastchart_series_t *s, int i)
{
    if (i >= s->len) return NAN;
    return s->values[i];
}

int fastchart_area_render_to_target(fastchart_area_obj *self, fastchart_target_t *t)
{
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
    /* Band mode requires exactly two series — the polygon is the
     * envelope between series[0] (upper) and series[1] (lower).
     * Falls back to the regular fill-to-baseline path if the caller
     * set band_mode without two series. */
    bool band = self->band_mode && n_series == 2 && !stacked;
    if (band) stacked = false;

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
    fastchart_compute_layout((fastchart_obj *)self, t, 1, 1, NULL, 0, &plot);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    fastchart_draw_frame(t, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_title(t, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_y_axis(t, (fastchart_obj *)self, &plot, &pal, &range);
    fastchart_draw_plot_bands(t, (fastchart_obj *)self, &plot, &range, &pal);
    fastchart_draw_v_plot_bands_categorical(t, (fastchart_obj *)self, &plot,
                                            max_len, &pal);

    const char **label_ptrs = fastchart_borrow_category_labels((fastchart_obj *)self, max_len);
    fastchart_draw_x_axis_categorical(t, (fastchart_obj *)self, &plot, &pal, max_len, label_ptrs);
    if (label_ptrs) efree((void *)label_ptrs);

    fastchart_draw_axis_titles(t, (fastchart_obj *)self, &plot, &pal);

    int alpha = (int)self->area_alpha;
    if (alpha < 0) alpha = 0;
    if (alpha > 127) alpha = 127;

    int edge_handle = self->edge_color >= 0
        ? fastchart_target_color_rgb(t, (int)self->edge_color) : -1;

    /* Build filled polygons. For stacked, accumulate per-category
     * sums; each series's polygon spans [prev_cum, prev_cum + v].
     * For non-stacked overlay, each series's polygon spans
     * [0, v] with translucent fill so layered shapes show through. */
    gdPoint poly[2 * FASTCHART_MAX_POINTS_PER_SERIES];

    if (band) {
        /* Build one closed polygon: series[0] left→right along the
         * top, then series[1] right→left along the bottom. The fill
         * is opaque (no per-series alpha layering here, the band
         * IS one shape). */
        int n_pts = 0;
        for (int i = 0; i < max_len && n_pts < 2048; i++) {
            double v = area_read_value(&series[0], i);
            if (isnan(v)) v = 0;
            int x = fastchart_x_categorical_center(&plot, i, max_len);
            int y = fastchart_y_to_pixel(v, &range, &plot);
            poly[n_pts].x = x; poly[n_pts].y = y;
            n_pts++;
        }
        for (int i = max_len - 1; i >= 0 && n_pts < 2 * 2048; i--) {
            double v = area_read_value(&series[1], i);
            if (isnan(v)) v = 0;
            int x = fastchart_x_categorical_center(&plot, i, max_len);
            int y = fastchart_y_to_pixel(v, &range, &plot);
            poly[n_pts].x = x; poly[n_pts].y = y;
            n_pts++;
        }
        int fill_handle = pal.series[0];
        uint32_t rgba = fastchart_target_color_to_rgba(t, fill_handle);
        int r = (rgba >> 16) & 0xFF;
        int g = (rgba >>  8) & 0xFF;
        int b =  rgba        & 0xFF;
        int alpha_byte = 255 - alpha * 2;
        if (alpha_byte < 0) alpha_byte = 0;
        int alpha_handle = fastchart_target_color(t, r, g, b, alpha_byte);
        if (n_pts >= 3) {
            fastchart_obj *base = (fastchart_obj *)self;
            if (base->gradient_from >= 0 && base->gradient_to >= 0) {
                uint32_t a = (uint32_t)alpha_byte & 0xFFu;
                uint32_t grad_from =
                    (a << 24) | ((uint32_t)base->gradient_from & 0xFFFFFFu);
                uint32_t grad_to =
                    (a << 24) | ((uint32_t)base->gradient_to   & 0xFFFFFFu);
                fastchart_target_gradient_polygon(t, poly, n_pts,
                    grad_from, grad_to, (int)base->gradient_dir);
            } else {
                fastchart_target_polygon(t, poly, n_pts, alpha_handle, 1, 0);
            }
            if (edge_handle >= 0) {
                fastchart_target_polygon(t, poly, n_pts, edge_handle, 0, 1);
            }
        }
        /* Stroke the two boundary curves so the band has crisp
         * upper and lower edges regardless of fill alpha. */
        for (int s = 0; s < 2; s++) {
            int stroke_handle = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            int prev_x = 0, prev_y = 0;
            bool prev_valid = false;
            for (int i = 0; i < max_len; i++) {
                double v = area_read_value(&series[s], i);
                if (isnan(v)) { prev_valid = false; continue; }
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                int y = fastchart_y_to_pixel(v, &range, &plot);
                if (prev_valid) {
                    fastchart_target_line(t, prev_x, prev_y, x, y,
                                          stroke_handle, 2, FASTCHART_DASH_SOLID);
                }
                prev_x = x; prev_y = y; prev_valid = true;
            }
        }
    } else if (stacked) {
        double *cum = ecalloc((size_t)max_len, sizeof(double));
        for (int s = 0; s < n_series; s++) {
            int series_handle = pal.series[s % FASTCHART_PALETTE_SERIES_N];
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
                fastchart_obj *base = (fastchart_obj *)self;
                if (base->gradient_from >= 0 && base->gradient_to >= 0) {
                    fastchart_target_gradient_polygon(t, poly, n_pts,
                        (uint32_t)base->gradient_from,
                        (uint32_t)base->gradient_to,
                        (int)base->gradient_dir);
                } else {
                    fastchart_target_polygon(t, poly, n_pts, series_handle, 1, 0);
                }
                if (edge_handle >= 0) {
                    fastchart_target_polygon(t, poly, n_pts, edge_handle, 0, 1);
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
                    fastchart_target_line(t, prev_x, prev_y, x, y,
                                          pal.border, 1, FASTCHART_DASH_SOLID);
                }
                prev_x = x; prev_y = y; prev_valid = true;
                cum[i] += v;
            }
        }
        efree(cum);
    } else {
        int zero_y = fastchart_y_to_pixel(dmin > 0 ? dmin : 0.0, &range, &plot);

        for (int s = 0; s < n_series; s++) {
            int base_handle = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            uint32_t rgba = fastchart_target_color_to_rgba(t, base_handle);
            int r = (rgba >> 16) & 0xFF;
            int g = (rgba >>  8) & 0xFF;
            int b =  rgba        & 0xFF;
            /* gd alpha 0..127 -> byte 255..1 via 255 - gd_alpha * 2. */
            int alpha_byte = 255 - alpha * 2;
            if (alpha_byte < 0) alpha_byte = 0;
            int alpha_handle = fastchart_target_color(t, r, g, b, alpha_byte);

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
                fastchart_obj *base = (fastchart_obj *)self;
                if (base->gradient_from >= 0 && base->gradient_to >= 0) {
                    /* Compose setAreaAlpha (0..127 gd convention,
                     * 127 = transparent) into the gradient stops so
                     * non-stacked overlay layers stay translucent
                     * the way solid-fill overlays do. */
                    uint32_t a = (uint32_t)alpha_byte & 0xFFu;
                    uint32_t grad_from =
                        (a << 24) | ((uint32_t)base->gradient_from & 0xFFFFFFu);
                    uint32_t grad_to =
                        (a << 24) | ((uint32_t)base->gradient_to   & 0xFFFFFFu);
                    fastchart_target_gradient_polygon(t, poly, n_pts,
                        grad_from, grad_to, (int)base->gradient_dir);
                } else {
                    fastchart_target_polygon(t, poly, n_pts, alpha_handle, 1, 0);
                }
                if (edge_handle >= 0) {
                    fastchart_target_polygon(t, poly, n_pts, edge_handle, 0, 1);
                }
            }

            /* Opaque top stroke. */
            int prev_x = 0, prev_y = 0;
            bool prev_valid = false;
            for (int i = 0; i < max_len; i++) {
                double v = area_read_value(&series[s], i);
                if (isnan(v)) { prev_valid = false; continue; }
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                int y = fastchart_y_to_pixel(v, &range, &plot);
                if (prev_valid) {
                    fastchart_target_line(t, prev_x, prev_y, x, y,
                                          base_handle, 2, FASTCHART_DASH_SOLID);
                }
                prev_x = x; prev_y = y; prev_valid = true;
            }
        }
    }

    /* Combo overlays + annotations on top of the area fills. */
    fastchart_draw_overlays_categorical(t, (fastchart_obj *)self, &plot, &pal,
                                         &range, NULL, max_len);

    fastchart_draw_h_annotations(t, (fastchart_obj *)self, &plot, &pal, &range);
    fastchart_draw_v_annotations_categorical(t, (fastchart_obj *)self, &plot, &pal, max_len);

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
            fastchart_draw_legend(t, (fastchart_obj *)self, &plot, &pal,
                                  legend_count, legend_colors, legend_labels);
        }
    }

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);

    if (self->icons && self->n_icons > 0 && max_len > 0) {
        for (int i = 0; i < self->n_icons; i++) {
            const fastchart_icon *ic = &self->icons[i];
            double frac_x = max_len > 1
                ? (ic->x + 0.5) / (double)max_len
                : 0.5;
            int px = plot.x0 + (int)(frac_x * (plot.x1 - plot.x0) + 0.5);
            int py = fastchart_y_to_pixel(ic->y, &range, &plot);
            fastchart_blit_icon(t, ic, px, py);
        }
    }
    return 0;
}

