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

#define MAX_SERIES 8

/* Bar chart data acceptance mirrors LineChart: flat numeric list for
 * single-series, [{label, data: [...]}] for multi-series. The two
 * concrete classes intentionally don't share C-side helpers so each
 * draw() is self-contained -- per the v0.1 scaffold AGENTS.md note,
 * per-type structs/handlers replace the shared shape later, and at
 * that point the data extraction will diverge anyway. */

typedef struct {
    HashTable *data;
    HashTable *colors;     /* optional per-bar colors */
    const char *label;
    int len;
} fastchart_bar_series;

static int collect_bar_series(zval *data_zv,
                              fastchart_bar_series *out,
                              int max_series,
                              int *out_count,
                              int *out_max_len)
{
    *out_count = 0;
    *out_max_len = 0;
    if (Z_TYPE_P(data_zv) != IS_ARRAY) return -1;
    HashTable *ht = Z_ARRVAL_P(data_zv);
    if (zend_hash_num_elements(ht) == 0) return 0;

    zval *first = zend_hash_index_find(ht, 0);
    int is_multi = 0;
    if (first && Z_TYPE_P(first) == IS_ARRAY) {
        zval *data_key = zend_hash_str_find(Z_ARRVAL_P(first), "data", sizeof("data") - 1);
        if (data_key && Z_TYPE_P(data_key) == IS_ARRAY) is_multi = 1;
    }

    if (is_multi) {
        zval *series_zv;
        ZEND_HASH_FOREACH_VAL(ht, series_zv) {
            if (Z_TYPE_P(series_zv) != IS_ARRAY) continue;
            zval *data_key = zend_hash_str_find(Z_ARRVAL_P(series_zv),
                                                "data", sizeof("data") - 1);
            if (!data_key || Z_TYPE_P(data_key) != IS_ARRAY) continue;
            if (*out_count >= max_series) break;

            HashTable *sht = Z_ARRVAL_P(data_key);
            out[*out_count].data = sht;
            out[*out_count].len = (int)zend_hash_num_elements(sht);

            zval *label_zv = zend_hash_str_find(Z_ARRVAL_P(series_zv),
                                                "label", sizeof("label") - 1);
            out[*out_count].label =
                (label_zv && Z_TYPE_P(label_zv) == IS_STRING)
                    ? Z_STRVAL_P(label_zv) : NULL;

            zval *colors_zv = zend_hash_str_find(Z_ARRVAL_P(series_zv),
                                                 "colors", sizeof("colors") - 1);
            out[*out_count].colors = (colors_zv && Z_TYPE_P(colors_zv) == IS_ARRAY)
                ? Z_ARRVAL_P(colors_zv) : NULL;

            if (out[*out_count].len > *out_max_len) *out_max_len = out[*out_count].len;
            (*out_count)++;
        } ZEND_HASH_FOREACH_END();
    } else {
        out[0].data = ht;
        out[0].colors = NULL;
        out[0].label = NULL;
        out[0].len = (int)zend_hash_num_elements(ht);
        *out_count = 1;
        *out_max_len = out[0].len;
    }
    return 0;
}

static int per_point_color(HashTable *colors_ht, int idx, int fallback,
                           gdImagePtr im)
{
    if (!colors_ht) return fallback;
    zval *cv = zend_hash_index_find(colors_ht, idx);
    if (!cv || Z_TYPE_P(cv) != IS_LONG) return fallback;
    long c = Z_LVAL_P(cv);
    if (c < 0 || c > 0xFFFFFF) return fallback;
    return gdImageColorAllocate(im,
        (int)((c >> 16) & 0xFF),
        (int)((c >>  8) & 0xFF),
        (int)( c        & 0xFF));
}

static int read_value(HashTable *ht, int index, double *out)
{
    zval *v = zend_hash_index_find(ht, index);
    if (!v) return -1;
    return fastchart_zval_to_double(v, out);
}

/* Read a [min, max] entry from a floating-bar series. Returns 0 on
 * success, -1 if the entry is missing or not a 2-element numeric
 * array. */
static int read_range(HashTable *ht, int index, double *lo, double *hi)
{
    zval *v = zend_hash_index_find(ht, index);
    if (!v || Z_TYPE_P(v) != IS_ARRAY) return -1;
    HashTable *pair = Z_ARRVAL_P(v);
    zval *zlo = zend_hash_index_find(pair, 0);
    zval *zhi = zend_hash_index_find(pair, 1);
    if (!zlo || !zhi) return -1;
    if (fastchart_zval_to_double(zlo, lo) != 0) return -1;
    if (fastchart_zval_to_double(zhi, hi) != 0) return -1;
    if (*lo > *hi) { double tmp = *lo; *lo = *hi; *hi = tmp; }
    return 0;
}

int fastchart_bar_render_to_image(fastchart_obj *self, gdImagePtr im)
{
    fastchart_bar_series series[MAX_SERIES];
    int n_series = 0, n_categories = 0;
    if (collect_bar_series(&self->data, series, MAX_SERIES,
                           &n_series, &n_categories) != 0 || n_series == 0) {
        zend_throw_error(NULL,
            "FastChart\\BarChart::draw() requires setSeries() to have been called with non-empty data");
        return -1;
    }

    bool stacked = false;
    {
        zval *cfg = zend_hash_str_find(Z_ARRVAL(self->config),
                                       "stacked", sizeof("stacked") - 1);
        if (cfg && Z_TYPE_P(cfg) == IS_TRUE) stacked = true;
    }
    /* setStackMode(STACK_LAYER) implies stacked-mode rendering with a
     * shared baseline + translucent fills; STACK_BESIDE mirrors
     * setStacked(false). */
    bool stack_layer = (self->stack_mode == FASTCHART_STACK_LAYER);
    if (self->stack_mode == FASTCHART_STACK_BESIDE) stacked = false;
    if (stack_layer && n_series > 1) stacked = true;
    bool floating = self->bar_floating;

    double dmin = 0, dmax = 0;
    int seen = 0;
    if (floating) {
        /* Floating bars: each entry is [min, max]; the value range
         * spans both ends with no zero-anchor (since the bar doesn't
         * start at the baseline). */
        for (int s = 0; s < n_series; s++) {
            for (int i = 0; i < n_categories; i++) {
                double lo, hi;
                if (read_range(series[s].data, i, &lo, &hi) != 0) {
                    if (self->strict) {
                        zend_type_error("FastChart strict mode: floating-bar entries must be [min, max]");
                        return -1;
                    }
                    continue;
                }
                if (!seen) { dmin = lo; dmax = hi; seen = 1; }
                else { if (lo < dmin) dmin = lo; if (hi > dmax) dmax = hi; }
            }
        }
    } else if (stacked && n_series > 1) {
        for (int i = 0; i < n_categories; i++) {
            double pos = 0, neg = 0;
            for (int s = 0; s < n_series; s++) {
                double v;
                if (read_value(series[s].data, i, &v) != 0) {
                    if (self->strict) {
                        zval *zv = zend_hash_index_find(series[s].data, i);
                        if (zv && Z_TYPE_P(zv) != IS_LONG && Z_TYPE_P(zv) != IS_DOUBLE) {
                            zend_type_error("FastChart strict mode: bar values must be numeric");
                            return -1;
                        }
                    }
                    continue;
                }
                if (v >= 0) pos += v; else neg += v;
            }
            if (!seen) { dmin = neg; dmax = pos; seen = 1; }
            else { if (pos > dmax) dmax = pos; if (neg < dmin) dmin = neg; }
        }
    } else {
        for (int s = 0; s < n_series; s++) {
            zval *v;
            ZEND_HASH_FOREACH_VAL(series[s].data, v) {
                double d;
                if (fastchart_zval_to_double(v, &d) != 0) {
                    if (self->strict) {
                        zend_type_error("FastChart strict mode: bar values must be numeric");
                        return -1;
                    }
                    continue;
                }
                if (!seen) { dmin = dmax = d; seen = 1; }
                else { if (d < dmin) dmin = d; if (d > dmax) dmax = d; }
            } ZEND_HASH_FOREACH_END();
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
        fastchart_value_range_apply_override(self, &range);
    }

    fastchart_rect plot;
    fastchart_compute_layout(self, im, 1, 1, &plot);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, self, &pal);

    fastchart_draw_frame(im, self, &plot, &pal);
    fastchart_draw_title(im, self, &plot, &pal);
    fastchart_draw_y_axis(im, self, &plot, &pal, &range);

    const char **label_ptrs = NULL;
    zval *labels_zv = zend_hash_str_find(Z_ARRVAL(self->config),
        "category_labels", sizeof("category_labels") - 1);
    if (labels_zv && Z_TYPE_P(labels_zv) == IS_ARRAY && n_categories > 0) {
        label_ptrs = ecalloc((size_t)n_categories, sizeof(const char *));
        for (int i = 0; i < n_categories; i++) {
            zval *lv = zend_hash_index_find(Z_ARRVAL_P(labels_zv), i);
            label_ptrs[i] = (lv && Z_TYPE_P(lv) == IS_STRING) ? Z_STRVAL_P(lv) : NULL;
        }
    }
    fastchart_draw_x_axis_categorical(im, self, &plot, &pal, n_categories, label_ptrs);
    if (label_ptrs) efree(label_ptrs);

    fastchart_draw_axis_titles(im, self, &plot, &pal);

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

    /* Allocate translucent series colors once for STACK_LAYER mode. */
    int layer_colors[MAX_SERIES] = {0};
    if (stack_layer && n_series > 1) {
        for (int s = 0; s < n_series; s++) {
            int c = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            int r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
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
                double lo, hi;
                if (read_range(series[s].data, i, &lo, &hi) != 0) continue;
                int series_color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
                int color = per_point_color(series[s].colors, i, series_color, im);
                int y_lo = fastchart_y_to_pixel(lo, &range, &plot);
                int y_hi = fastchart_y_to_pixel(hi, &range, &plot);
                int y0 = y_hi < y_lo ? y_hi : y_lo;
                int y1 = y_hi < y_lo ? y_lo : y_hi;
                int x0 = slot_left + s * sub_w + sub_inset;
                int x1 = x0 + draw_w - 1;
                if (x1 > slot_left + slot_inner - 1) x1 = slot_left + slot_inner - 1;
                fastchart_shadow_filled_rectangle(im, self, x0, y0, x1, y1);
                if (!fastchart_gradient_filled_rectangle(im, self, x0, y0, x1, y1)) {
                    gdImageFilledRectangle(im, x0, y0, x1, y1, color);
                }
                if (edge >= 0) gdImageRectangle(im, x0, y0, x1, y1, edge);
            }
        } else if (stack_layer && n_series > 1) {
            /* Layered: all series anchor at zero with translucent
             * fills, painter overlay rather than cumulative. */
            for (int s = 0; s < n_series; s++) {
                double v;
                if (read_value(series[s].data, i, &v) != 0) continue;
                int color = layer_colors[s];
                int y_v = fastchart_y_to_pixel(v, &range, &plot);
                int y0 = y_v < zero_y ? y_v : zero_y;
                int y1 = y_v < zero_y ? zero_y : y_v;
                int x0 = slot_left + sub_inset;
                int x1 = x0 + draw_w - 1;
                if (x1 > slot_left + slot_inner - 1) x1 = slot_left + slot_inner - 1;
                fastchart_shadow_filled_rectangle(im, self, x0, y0, x1, y1);
                if (!fastchart_gradient_filled_rectangle(im, self, x0, y0, x1, y1)) {
                    gdImageFilledRectangle(im, x0, y0, x1, y1, color);
                }
                if (edge >= 0) gdImageRectangle(im, x0, y0, x1, y1, edge);
            }
        } else if (stacked && n_series > 1) {
            double pos_acc = 0, neg_acc = 0;
            for (int s = 0; s < n_series; s++) {
                double v;
                if (read_value(series[s].data, i, &v) != 0) continue;
                int series_color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
                int color = per_point_color(series[s].colors, i, series_color, im);

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
                fastchart_shadow_filled_rectangle(im, self, x0, y0, x1, y1);
                if (!fastchart_gradient_filled_rectangle(im, self, x0, y0, x1, y1)) {
                    gdImageFilledRectangle(im, x0, y0, x1, y1, color);
                }
                if (edge >= 0) gdImageRectangle(im, x0, y0, x1, y1, edge);
            }
        } else {
            for (int s = 0; s < n_series; s++) {
                double v;
                if (read_value(series[s].data, i, &v) != 0) continue;
                int series_color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
                int color = per_point_color(series[s].colors, i, series_color, im);
                int y_v = fastchart_y_to_pixel(v, &range, &plot);

                int x0 = slot_left + s * sub_w + sub_inset;
                int x1 = x0 + draw_w - 1;
                if (x1 > slot_left + slot_inner - 1) x1 = slot_left + slot_inner - 1;

                int y0 = y_v < zero_y ? y_v : zero_y;
                int y1 = y_v < zero_y ? zero_y : y_v;
                fastchart_shadow_filled_rectangle(im, self, x0, y0, x1, y1);
                if (!fastchart_gradient_filled_rectangle(im, self, x0, y0, x1, y1)) {
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
                double v;
                if (read_value(series[s].data, i, &v) != 0) continue;
                int y_v = fastchart_y_to_pixel(v, &range, &plot);
                int x0 = slot_left + s * sub_w;
                int x_center = x0 + sub_w / 2;
                /* Label sits just above the bar top (or below for
                 * negative bars). */
                int label_y = (v >= 0) ? y_v : y_v + (int)(self->font_size * 1.4);
                fastchart_draw_value_label(im, self, &pal, x_center, label_y, v);
            }
        }
    }

    fastchart_draw_overlays_categorical(im, self, &plot, &pal,
                                         &range, NULL, n_categories);

    fastchart_draw_h_annotations(im, self, &plot, &pal, &range);
    fastchart_draw_v_annotations_categorical(im, self, &plot, &pal, n_categories);

    if (n_series >= 2) {
        int legend_colors[MAX_SERIES];
        const char *legend_labels[MAX_SERIES];
        int legend_count = 0;
        for (int s = 0; s < n_series; s++) {
            if (!series[s].label) continue;
            legend_colors[legend_count] = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            legend_labels[legend_count] = series[s].label;
            legend_count++;
        }
        if (legend_count > 0) {
            fastchart_draw_legend(im, self, &plot, &pal,
                                  legend_count, legend_colors, legend_labels);
        }
    }

    fastchart_draw_text_annotations(im, self, &pal);
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

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (fastchart_bar_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
