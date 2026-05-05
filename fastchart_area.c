/*
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2026 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,     |
  | that is bundled with this package in the file LICENSE, and is       |
  | available through the world-wide-web at the following url:          |
  | http://www.php.net/license/3_01.txt                                 |
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
#include "fastchart_axis.h"

#define MAX_SERIES 8

typedef struct {
    HashTable *data;
    const char *label;
    bool right_axis;
    int len;
} fastchart_area_series;

static int collect_area_series(zval *data_zv,
                               fastchart_area_series *out,
                               int max_series,
                               int *out_count,
                               int *out_max_len)
{
    *out_count = 0;
    *out_max_len = 0;

    if (Z_TYPE_P(data_zv) != IS_ARRAY) return -1;
    HashTable *ht = Z_ARRVAL_P(data_zv);
    int n = (int)zend_hash_num_elements(ht);
    if (n == 0) return 0;

    zval *first = zend_hash_index_find(ht, 0);
    int is_multi = 0;
    if (first && Z_TYPE_P(first) == IS_ARRAY) {
        zval *data_key = zend_hash_str_find(Z_ARRVAL_P(first),
                                            "data", sizeof("data") - 1);
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
            out[*out_count].label = (label_zv && Z_TYPE_P(label_zv) == IS_STRING)
                ? Z_STRVAL_P(label_zv) : NULL;

            zval *axis_zv = zend_hash_str_find(Z_ARRVAL_P(series_zv),
                                               "axis", sizeof("axis") - 1);
            out[*out_count].right_axis =
                (axis_zv && Z_TYPE_P(axis_zv) == IS_STRING &&
                 strcmp(Z_STRVAL_P(axis_zv), "right") == 0);

            if (out[*out_count].len > *out_max_len) *out_max_len = out[*out_count].len;
            (*out_count)++;
        } ZEND_HASH_FOREACH_END();
    } else {
        out[0].data = ht;
        out[0].label = NULL;
        out[0].right_axis = false;
        out[0].len = n;
        *out_count = 1;
        *out_max_len = n;
    }
    return 0;
}

/* Read value at index, returning NaN on null/non-numeric (gap). */
static double read_value_or_nan(HashTable *ht, int idx, bool strict, int *err)
{
    zval *v = zend_hash_index_find(ht, idx);
    if (!v) return NAN;
    if (Z_TYPE_P(v) == IS_NULL) return NAN;
    double d;
    if (fastchart_zval_to_double(v, &d) == 0) return d;
    if (strict) {
        zend_type_error("FastChart strict mode: area data must be numeric or null");
        *err = 1;
    }
    return NAN;
}

int fastchart_area_render_to_image(fastchart_obj *self, gdImagePtr im)
{
    fastchart_area_series series[MAX_SERIES];
    int n_series = 0, max_len = 0;
    if (collect_area_series(&self->data, series, MAX_SERIES,
                            &n_series, &max_len) != 0 || n_series == 0) {
        zend_throw_error(NULL,
            "FastChart\\AreaChart::draw() requires setSeries() to have been called with non-empty data");
        return -1;
    }

    bool stacked = false;
    {
        zval *cfg = zend_hash_str_find(Z_ARRVAL(self->config),
                                       "stacked", sizeof("stacked") - 1);
        if (cfg && Z_TYPE_P(cfg) == IS_TRUE) stacked = true;
    }
    if (n_series < 2) stacked = false;

    /* Y range. For stacked, max is the cumulative sum; min anchors
     * at 0 (negative areas don't make stacked sense in v0.1). */
    double dmin = 0, dmax = 0;
    int seen = 0;
    int err_strict = 0;

    if (stacked) {
        for (int i = 0; i < max_len; i++) {
            double sum = 0;
            for (int s = 0; s < n_series; s++) {
                double v = read_value_or_nan(series[s].data, i, self->strict, &err_strict);
                if (err_strict) return -1;
                if (!isnan(v)) sum += v;
            }
            if (!seen) { dmin = 0; dmax = sum; seen = 1; }
            else if (sum > dmax) dmax = sum;
        }
    } else {
        for (int s = 0; s < n_series; s++) {
            zval *v;
            ZEND_HASH_FOREACH_VAL(series[s].data, v) {
                if (Z_TYPE_P(v) == IS_NULL) continue;
                double d;
                if (fastchart_zval_to_double(v, &d) != 0) {
                    if (self->strict) {
                        zend_type_error("FastChart strict mode: area data must be numeric or null");
                        return -1;
                    }
                    continue;
                }
                if (!seen) { dmin = dmax = d; seen = 1; }
                else { if (d < dmin) dmin = d; if (d > dmax) dmax = d; }
            } ZEND_HASH_FOREACH_END();
        }
        if (dmin > 0) dmin = 0;  /* anchor at zero so the fill is meaningful */
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

    /* Categorical X axis (same shape as LineChart). */
    const char **label_ptrs = NULL;
    zval *labels_zv = zend_hash_str_find(Z_ARRVAL(self->config),
        "category_labels", sizeof("category_labels") - 1);
    if (labels_zv && Z_TYPE_P(labels_zv) == IS_ARRAY && max_len > 0) {
        label_ptrs = ecalloc((size_t)max_len, sizeof(const char *));
        for (int i = 0; i < max_len; i++) {
            zval *lv = zend_hash_index_find(Z_ARRVAL_P(labels_zv), i);
            label_ptrs[i] = (lv && Z_TYPE_P(lv) == IS_STRING) ? Z_STRVAL_P(lv) : NULL;
        }
    }
    fastchart_draw_x_axis_categorical(im, self, &plot, &pal, max_len, label_ptrs);
    if (label_ptrs) efree(label_ptrs);

    fastchart_draw_axis_titles(im, self, &plot, &pal);

    int alpha = (int)self->area_alpha;
    if (alpha < 0) alpha = 0;
    if (alpha > 127) alpha = 127;

    /* Build filled polygons. For stacked, accumulate per-category
     * sums; each series's polygon spans [prev_cum, prev_cum + v].
     * For non-stacked overlay, each series's polygon spans
     * [0, v] with translucent fill so layered shapes show through. */
    gdPoint poly[2 * 2048];

    if (stacked) {
        double *cum = ecalloc((size_t)max_len, sizeof(double));
        for (int s = 0; s < n_series; s++) {
            int rgb_color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            int n_pts = 0;

            /* Top edge: left to right at cum + v. */
            for (int i = 0; i < max_len && n_pts < 2048; i++) {
                double v = read_value_or_nan(series[s].data, i, false, &err_strict);
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
                gdImageFilledPolygon(im, poly, n_pts, rgb_color);
            }

            /* Top-edge stroke for crisp boundary between layers. */
            int prev_x = 0, prev_y = 0;
            bool prev_valid = false;
            for (int i = 0; i < max_len; i++) {
                double v = read_value_or_nan(series[s].data, i, false, &err_strict);
                if (isnan(v)) v = 0;
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                int y = fastchart_y_to_pixel(cum[i] + v, &range, &plot);
                if (prev_valid) {
                    gdImageLine(im, prev_x, prev_y, x, y, pal.border);
                }
                prev_x = x; prev_y = y; prev_valid = true;
                cum[i] += v;
            }
        }
        efree(cum);
    } else {
        int zero_y = fastchart_y_to_pixel(dmin > 0 ? dmin : 0.0, &range, &plot);

        for (int s = 0; s < n_series; s++) {
            int base_color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            /* Allocate a translucent variant of the series color for
             * the fill. The line stroke uses the opaque version. */
            int fill_color = gdImageColorAllocateAlpha(im,
                (base_color >> 16) & 0xFF,  /* placeholder; unused */
                0, 0, 0);
            (void)fill_color;
            /* Re-extract RGB from the gd palette since pal.series
             * stores allocated indices, not raw RGB. We allocated the
             * series colors via fastchart_palette_init, so the raw
             * RGB of slot s comes from gdImageRed/Green/Blue. */
            int r = gdImageRed(im, base_color);
            int g = gdImageGreen(im, base_color);
            int b = gdImageBlue(im, base_color);
            int alpha_color = gdImageColorAllocateAlpha(im, r, g, b, alpha);

            int n_pts = 0;
            for (int i = 0; i < max_len && n_pts < 2048; i++) {
                double v = read_value_or_nan(series[s].data, i, false, &err_strict);
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
                gdImageAlphaBlending(im, 1);
                gdImageFilledPolygon(im, poly, n_pts, alpha_color);
                gdImageAlphaBlending(im, 0);
            }

            /* Opaque top stroke. */
            gdImageSetThickness(im, 2);
            gdImageSetAntiAliased(im, base_color);
            int prev_x = 0, prev_y = 0;
            bool prev_valid = false;
            for (int i = 0; i < max_len; i++) {
                double v = read_value_or_nan(series[s].data, i, false, &err_strict);
                if (isnan(v)) { prev_valid = false; continue; }
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                int y = fastchart_y_to_pixel(v, &range, &plot);
                if (prev_valid) gdImageLine(im, prev_x, prev_y, x, y, gdAntiAliased);
                prev_x = x; prev_y = y; prev_valid = true;
            }
            gdImageSetThickness(im, 1);
        }
    }

    fastchart_draw_h_annotations(im, self, &plot, &pal, &range);
    fastchart_draw_v_annotations_categorical(im, self, &plot, &pal, max_len);

    /* Legend. */
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

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (fastchart_area_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
