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

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_axis.h"

/* Walk the data zval and resolve into a list of series. The accepted
 * formats:
 *   1. Flat list of numbers: [1, 4, 2, 8]      -> 1 series, length 4
 *   2. List of series:  [{label, data: [...]}, ...]
 *
 * Returns an array of HashTable* pointing at each series's data array;
 * caller indexes via the returned count. labels[i] is the label of
 * series i (NULL if absent). max_n is the longest series length.
 *
 * Sets *out_count to 0 if the data zval is empty or shape-invalid. */
typedef struct {
    HashTable *data;        /* zend_hash; values are numeric */
    const char *label;      /* NULL if the series had no label */
    int len;
} fastchart_line_series;

static int collect_line_series(zval *data_zv,
                               fastchart_line_series *out,
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

    /* Decide format: peek at the first element. If it's an array AND
     * has a 'data' key OR contains nested numeric arrays, treat as
     * multi-series. Otherwise treat the whole thing as one flat
     * numeric series. */
    zval *first = zend_hash_index_find(ht, 0);
    if (!first) {
        /* Associative; not a list. Treat as single flat series. */
        first = NULL;
    }

    int is_multi = 0;
    if (first && Z_TYPE_P(first) == IS_ARRAY) {
        zval *data_key = zend_hash_str_find(Z_ARRVAL_P(first), "data", sizeof("data") - 1);
        if (data_key && Z_TYPE_P(data_key) == IS_ARRAY) {
            is_multi = 1;
        }
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
            if (label_zv && Z_TYPE_P(label_zv) == IS_STRING) {
                out[*out_count].label = Z_STRVAL_P(label_zv);
            } else {
                out[*out_count].label = NULL;
            }

            if (out[*out_count].len > *out_max_len) {
                *out_max_len = out[*out_count].len;
            }
            (*out_count)++;
        } ZEND_HASH_FOREACH_END();
    } else {
        out[0].data = ht;
        out[0].label = NULL;
        out[0].len = n;
        *out_count = 1;
        *out_max_len = n;
    }

    return 0;
}

/* Iterate values of `series_data` in insertion order, storing
 * coerced doubles into `out` (which has at least `len` slots). Returns
 * the number of values successfully extracted (may be less than the
 * length if a non-numeric snuck in -- which we just skip). */
static int extract_doubles(HashTable *series_data, double *out, int max_n)
{
    int n = 0;
    zval *v;
    ZEND_HASH_FOREACH_VAL(series_data, v) {
        if (n >= max_n) break;
        double d;
        if (fastchart_zval_to_double(v, &d) == 0) {
            out[n++] = d;
        }
    } ZEND_HASH_FOREACH_END();
    return n;
}

/* Find the data min and max across all series. Returns 0 if at least
 * one numeric value was seen, -1 otherwise. */
static int compute_y_range(fastchart_line_series *series, int n_series,
                           double *out_min, double *out_max)
{
    int seen = 0;
    double mn = 0, mx = 0;
    for (int s = 0; s < n_series; s++) {
        zval *v;
        ZEND_HASH_FOREACH_VAL(series[s].data, v) {
            double d;
            if (fastchart_zval_to_double(v, &d) != 0) continue;
            if (!seen) {
                mn = mx = d;
                seen = 1;
            } else {
                if (d < mn) mn = d;
                if (d > mx) mx = d;
            }
        } ZEND_HASH_FOREACH_END();
    }
    if (!seen) return -1;
    *out_min = mn;
    *out_max = mx;
    return 0;
}

#define MAX_SERIES 8

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

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);

    fastchart_line_series series[MAX_SERIES];
    int n_series = 0, max_len = 0;
    if (collect_line_series(&self->data, series, MAX_SERIES,
                            &n_series, &max_len) != 0 || n_series == 0) {
        zend_throw_error(NULL,
            "FastChart\\LineChart::draw() requires setSeries() to have been called with non-empty data");
        RETURN_THROWS();
    }

    double dmin, dmax;
    if (compute_y_range(series, n_series, &dmin, &dmax) != 0) {
        zend_throw_error(NULL,
            "FastChart\\LineChart::draw() found no numeric values in the series");
        RETURN_THROWS();
    }

    fastchart_value_range range;
    fastchart_value_range_compute(dmin, dmax, 6, &range);

    fastchart_rect plot;
    fastchart_compute_layout(self, im, 1, 1, &plot);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);

    fastchart_draw_frame(im, self, &plot, &pal);
    fastchart_draw_title(im, self, &plot, &pal);
    fastchart_draw_y_axis(im, self, &plot, &pal, &range);

    /* Pull category labels out of config if setCategoryLabels() was
     * called. The PHP zend_string buffers stay alive for the
     * duration of this method call (config is rooted in self), so
     * keeping borrowed const char* into them inside the labels
     * array is safe. */
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

    /* Each series is rendered as line segments connecting consecutive
     * points, plus a small marker at every data point. Line width is
     * 2px via gdImageSetThickness so the series stand out against the
     * grid; reset to 1px before any axis-overdraw work would happen.
     *
     * gdAntiAliased is a sentinel color that draws using whatever
     * color was last passed to gdImageSetAntiAliased. Set it per-
     * series so segments smooth out diagonally without bleed-through
     * between adjacent series colors. */
    gdImageSetThickness(im, 2);

    int legend_colors[MAX_SERIES];
    const char *legend_labels[MAX_SERIES];
    int legend_count = 0;

    double values[2048];
    for (int s = 0; s < n_series; s++) {
        int color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
        int n = extract_doubles(series[s].data, values,
                                series[s].len < 2048 ? series[s].len : 2048);
        if (n < 1) continue;

        gdImageSetAntiAliased(im, color);
        int prev_x = 0, prev_y = 0;
        for (int i = 0; i < n; i++) {
            int x = fastchart_x_categorical_center(&plot, i, max_len);
            int y = fastchart_y_to_pixel(values[i], &range, &plot);

            if (i > 0) {
                gdImageLine(im, prev_x, prev_y, x, y, gdAntiAliased);
            }
            /* Filled marker disk. Radius 3 -- visible without being
             * obtrusive on a 800x600 default canvas. */
            gdImageFilledEllipse(im, x, y, 6, 6, color);

            prev_x = x;
            prev_y = y;
        }

        if (series[s].label) {
            legend_colors[legend_count] = color;
            legend_labels[legend_count] = series[s].label;
            legend_count++;
        }
    }
    gdImageSetThickness(im, 1);

    /* Legend only fires when at least one series carried a label.
     * Single-series flat-list inputs have no label so they don't
     * paint a one-row legend, which would just be visual noise. */
    if (legend_count >= 1 && n_series >= 2) {
        fastchart_draw_legend(im, self, &plot, &pal,
                              legend_count, legend_colors, legend_labels);
    }

    RETURN_ZVAL(canvas_zv, 1, 0);
}
