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
#include "fastchart_effects.h"

/* Resolve a per-point RGB into a gd color handle, falling back to
 * the series default. setSeries() validated each entry to 0..0xFFFFFF
 * or stored -1; we just allocate. */
static int bar_per_point_color(zend_long *point_colors, int idx, int fallback,
                               gdImagePtr im)
{
    if (!point_colors) return fallback;
    zend_long c = point_colors[idx];
    if (c < 0) return fallback;
    return gdImageColorAllocate(im,
        (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
}

int fastchart_bar_render_to_image(fastchart_bar_obj *self, gdImagePtr im)
{
    if (self->n_series == 0) {
        zend_throw_error(NULL,
            "FastChart\\BarChart::draw() requires setSeries() to have been called with non-empty data");
        return -1;
    }
    fastchart_series_t *series = self->series;
    int n_series = self->n_series;
    int n_categories = self->max_len;

    bool stacked = false;
    {
        zval *cfg = zend_hash_str_find(Z_ARRVAL(self->config),
                                       "stacked", sizeof("stacked") - 1);
        if (cfg && Z_TYPE_P(cfg) == IS_TRUE) stacked = true;
    }
    bool stack_layer = (self->stack_mode == FASTCHART_STACK_LAYER);
    if (self->stack_mode == FASTCHART_STACK_BESIDE) stacked = false;
    if (stack_layer && n_series > 1) stacked = true;
    bool floating = self->bar_floating;

    double dmin = 0, dmax = 0;
    int seen = 0;
    if (floating) {
        for (int s = 0; s < n_series; s++) {
            for (int i = 0; i < series[s].len; i++) {
                double lo = series[s].values[i];
                double hi = series[s].values_max ? series[s].values_max[i] : NAN;
                if (isnan(lo) || isnan(hi)) continue;
                if (!seen) { dmin = lo; dmax = hi; seen = 1; }
                else { if (lo < dmin) dmin = lo; if (hi > dmax) dmax = hi; }
            }
        }
    } else if (stacked && n_series > 1) {
        for (int i = 0; i < n_categories; i++) {
            double pos = 0, neg = 0;
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double v = series[s].values[i];
                if (isnan(v)) continue;
                if (v >= 0) pos += v; else neg += v;
            }
            if (!seen) { dmin = neg; dmax = pos; seen = 1; }
            else { if (pos > dmax) dmax = pos; if (neg < dmin) dmin = neg; }
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
    }
    if (!seen) {
        zend_throw_error(NULL,
            "FastChart\\BarChart::draw() found no numeric values in the series");
        return -1;
    }
    /* Floating bars don't anchor at zero. Regular bars do. */
    if (!floating) {
        if (dmin > 0) dmin = 0;
        if (dmax < 0) dmax = 0;
    }

    fastchart_value_range range;
    if (self->y_axis_scale == FASTCHART_SCALE_LOG) {
        if (dmin <= 0) {
            zend_value_error("FastChart\\BarChart::draw(): log Y-axis requires strictly-positive data (bars anchor at 0)");
            return -1;
        }
        if (fastchart_value_range_compute_log(dmin, dmax, &range) != 0) {
            zend_value_error("FastChart\\BarChart::draw(): log Y-axis requires strictly-positive data");
            return -1;
        }
    } else {
        fastchart_value_range_compute(dmin, dmax, 6, &range);
        fastchart_value_range_apply_override((fastchart_obj *)self, &range);
    }

    fastchart_rect plot;
    fastchart_compute_layout((fastchart_obj *)self, im, 1, 1, &plot);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, (fastchart_obj *)self, &pal);

    fastchart_draw_frame(im, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_title(im, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_y_axis(im, (fastchart_obj *)self, &plot, &pal, &range);

    const char **label_ptrs = NULL;
    zval *labels_zv = zend_hash_str_find(Z_ARRVAL(self->config),
        "category_labels", sizeof("category_labels") - 1);
    if (labels_zv && Z_TYPE_P(labels_zv) == IS_ARRAY && n_categories > 0) {
        label_ptrs = ecalloc((size_t)n_categories, sizeof(const char *));
        for (int i = 0; i < n_categories; i++) {
            zval *lv = zend_hash_index_find(Z_ARRVAL_P(labels_zv), i);
            label_ptrs[i] = fastchart_label_or_null(lv);
        }
    }
    fastchart_draw_x_axis_categorical(im, (fastchart_obj *)self, &plot, &pal, n_categories, label_ptrs);
    if (label_ptrs) efree(label_ptrs);

    fastchart_draw_axis_titles(im, (fastchart_obj *)self, &plot, &pal);

    int zero_y = fastchart_y_to_pixel(0.0, &range, &plot);

    int slot_w = (plot.x1 - plot.x0) / (n_categories > 0 ? n_categories : 1);
    int slot_pad = slot_w / 6;
    if (slot_pad < 1) slot_pad = 1;
    int slot_inner = slot_w - 2 * slot_pad;
    if (slot_inner < 1) slot_inner = 1;

    int sub_count = (stacked && n_series > 1) ? 1 : n_series;
    int sub_w = slot_inner / sub_count;
    if (sub_w < 1) sub_w = 1;

    /* setBarWidth(pct) shrinks the bar fill within its allocated
     * sub-slot, centered, at pct/100 of the slot width. 100 = touch
     * neighbors, 50 = half-width with breathing room. Applied to
     * sub_w so per-series side-by-side bars all narrow together. */
    int bar_pct = (int)self->bar_width_pct;
    if (bar_pct <= 0) bar_pct = 100;
    int draw_w = (sub_w * bar_pct + 50) / 100;
    if (draw_w < 1) draw_w = 1;
    int sub_inset = (sub_w - draw_w) / 2;

    int edge = (int)self->edge_color;

    /* Allocate translucent series colors once for STACK_LAYER mode.
     * pal.series[s] is a gd color handle (the return of
     * gdImageColorAllocate); on a paletted canvas that's an index
     * into the palette, not a packed 0xRRGGBB. Use the gdImageRed/
     * Green/Blue accessors so the unpack works for both truecolor
     * and paletted GdImage canvases. */
    int layer_colors[FASTCHART_MAX_SERIES] = {0};
    if (stack_layer && n_series > 1) {
        for (int s = 0; s < n_series; s++) {
            int c = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            int r = gdImageRed(im, c);
            int g = gdImageGreen(im, c);
            int b = gdImageBlue(im, c);
            layer_colors[s] = gdImageColorAllocateAlpha(im, r, g, b, 64);
        }
        gdImageAlphaBlending(im, 1);
    }

    for (int i = 0; i < n_categories; i++) {
        int slot_left = plot.x0 + i * slot_w + slot_pad;

        if (floating) {
            /* Floating bar: each series carries [min, max] per slot;
             * draw between min and max instead of from zero. */
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double lo = series[s].values[i];
                double hi = series[s].values_max ? series[s].values_max[i] : NAN;
                if (isnan(lo) || isnan(hi)) continue;
                int series_color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
                int color = bar_per_point_color(series[s].point_colors, i, series_color, im);
                int y_lo = fastchart_y_to_pixel(lo, &range, &plot);
                int y_hi = fastchart_y_to_pixel(hi, &range, &plot);
                int y0 = y_hi < y_lo ? y_hi : y_lo;
                int y1 = y_hi < y_lo ? y_lo : y_hi;
                int x0 = slot_left + s * sub_w + sub_inset;
                int x1 = x0 + draw_w - 1;
                if (x1 > slot_left + slot_inner - 1) x1 = slot_left + slot_inner - 1;
                fastchart_shadow_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1);
                if (!fastchart_gradient_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1)) {
                    gdImageFilledRectangle(im, x0, y0, x1, y1, color);
                }
                if (edge >= 0) gdImageRectangle(im, x0, y0, x1, y1, edge);
            }
        } else if (stack_layer && n_series > 1) {
            /* Layered: all series anchor at zero with translucent
             * fills, painter overlay rather than cumulative. */
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double v = series[s].values[i];
                if (isnan(v)) continue;
                int color = layer_colors[s];
                int y_v = fastchart_y_to_pixel(v, &range, &plot);
                int y0 = y_v < zero_y ? y_v : zero_y;
                int y1 = y_v < zero_y ? zero_y : y_v;
                int x0 = slot_left + sub_inset;
                int x1 = x0 + draw_w - 1;
                if (x1 > slot_left + slot_inner - 1) x1 = slot_left + slot_inner - 1;
                fastchart_shadow_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1);
                if (!fastchart_gradient_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1)) {
                    gdImageFilledRectangle(im, x0, y0, x1, y1, color);
                }
                if (edge >= 0) gdImageRectangle(im, x0, y0, x1, y1, edge);
            }
        } else if (stacked && n_series > 1) {
            double pos_acc = 0, neg_acc = 0;
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double v = series[s].values[i];
                if (isnan(v)) continue;
                int series_color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
                int color = bar_per_point_color(series[s].point_colors, i, series_color, im);

                double a, b;
                if (v >= 0) {
                    a = pos_acc; b = pos_acc + v; pos_acc = b;
                } else {
                    a = neg_acc + v; b = neg_acc; neg_acc = a;
                }
                int y_a = fastchart_y_to_pixel(a, &range, &plot);
                int y_b = fastchart_y_to_pixel(b, &range, &plot);
                int y0 = y_a < y_b ? y_a : y_b;
                int y1 = y_a < y_b ? y_b : y_a;
                int x0 = slot_left + sub_inset;
                int x1 = x0 + draw_w - 1;
                if (x1 > slot_left + slot_inner - 1) x1 = slot_left + slot_inner - 1;
                fastchart_shadow_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1);
                if (!fastchart_gradient_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1)) {
                    gdImageFilledRectangle(im, x0, y0, x1, y1, color);
                }
                if (edge >= 0) gdImageRectangle(im, x0, y0, x1, y1, edge);
            }
        } else {
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double v = series[s].values[i];
                if (isnan(v)) continue;
                int series_color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
                int color = bar_per_point_color(series[s].point_colors, i, series_color, im);
                int y_v = fastchart_y_to_pixel(v, &range, &plot);

                int x0 = slot_left + s * sub_w + sub_inset;
                int x1 = x0 + draw_w - 1;
                if (x1 > slot_left + slot_inner - 1) x1 = slot_left + slot_inner - 1;

                int y0 = y_v < zero_y ? y_v : zero_y;
                int y1 = y_v < zero_y ? zero_y : y_v;
                fastchart_shadow_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1);
                if (!fastchart_gradient_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1)) {
                    gdImageFilledRectangle(im, x0, y0, x1, y1, color);
                }
                if (edge >= 0) gdImageRectangle(im, x0, y0, x1, y1, edge);
            }
        }
    }

    if (range.min < 0 && range.max > 0) {
        gdImageLine(im, plot.x0, zero_y, plot.x1, zero_y, pal.axis);
    }

    /* Value labels above each bar (skipped when stacked since the
     * label would land mid-stack). */
    if (self->show_values && !(stacked && n_series > 1)) {
        for (int i = 0; i < n_categories; i++) {
            int slot_left = plot.x0 + i * slot_w + slot_pad;
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double v = series[s].values[i];
                if (isnan(v)) continue;
                int y_v = fastchart_y_to_pixel(v, &range, &plot);
                int x0 = slot_left + s * sub_w;
                int x_center = x0 + sub_w / 2;
                /* Label sits just above the bar top (or below for
                 * negative bars). */
                int label_y = (v >= 0) ? y_v : y_v + (int)(self->font_size * 1.4);
                fastchart_draw_value_label(im, (fastchart_obj *)self, &pal, x_center, label_y, v);
            }
        }
    }

    fastchart_draw_overlays_categorical(im, (fastchart_obj *)self, &plot, &pal,
                                         &range, NULL, n_categories);

    fastchart_draw_h_annotations(im, (fastchart_obj *)self, &plot, &pal, &range);
    fastchart_draw_v_annotations_categorical(im, (fastchart_obj *)self, &plot, &pal, n_categories);

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
            fastchart_draw_legend(im, (fastchart_obj *)self, &plot, &pal,
                                  legend_count, legend_colors, legend_labels);
        }
    }

    fastchart_draw_text_annotations(im, (fastchart_obj *)self, &pal);
    return 0;
}

ZEND_METHOD(FastChart_BarChart, draw)
{
    zval *canvas_zv;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\BarChart::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }

    fastchart_bar_obj *self = Z_FASTCHART_BAR_OBJ_P(ZEND_THIS);
    if (fastchart_bar_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
