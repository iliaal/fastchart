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
    HashTable *data;        /* zend_hash; values are numeric or NULL (gap) */
    HashTable *colors;      /* optional per-point colors; NULL when absent */
    const char *label;      /* NULL if the series had no label */
    bool right_axis;        /* opt-in to setSecondaryYAxis() right scale */
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
            out[*out_count].label = (label_zv && Z_TYPE_P(label_zv) == IS_STRING)
                ? Z_STRVAL_P(label_zv) : NULL;

            zval *colors_zv = zend_hash_str_find(Z_ARRVAL_P(series_zv),
                                                 "colors", sizeof("colors") - 1);
            out[*out_count].colors = (colors_zv && Z_TYPE_P(colors_zv) == IS_ARRAY)
                ? Z_ARRVAL_P(colors_zv) : NULL;

            zval *axis_zv = zend_hash_str_find(Z_ARRVAL_P(series_zv),
                                               "axis", sizeof("axis") - 1);
            out[*out_count].right_axis =
                (axis_zv && Z_TYPE_P(axis_zv) == IS_STRING &&
                 strcmp(Z_STRVAL_P(axis_zv), "right") == 0);

            if (out[*out_count].len > *out_max_len) {
                *out_max_len = out[*out_count].len;
            }
            (*out_count)++;
        } ZEND_HASH_FOREACH_END();
    } else {
        out[0].data = ht;
        out[0].colors = NULL;
        out[0].label = NULL;
        out[0].right_axis = false;
        out[0].len = n;
        *out_count = 1;
        *out_max_len = n;
    }

    return 0;
}

/* Iterate values of `series_data`, storing coerced doubles into
 * `out[i]`. NULL values become NaN (intentional gap). Numeric values
 * pass through. Other types: NaN in non-strict mode, TypeError in
 * strict mode. Returns the number of slots filled (up to max_n), or
 * -1 if strict mode rejected a value. */
static int extract_doubles(HashTable *series_data, double *out, int max_n,
                           bool strict)
{
    int n = 0;
    zval *v;
    ZEND_HASH_FOREACH_VAL(series_data, v) {
        if (n >= max_n) break;
        if (Z_TYPE_P(v) == IS_NULL) {
            out[n++] = NAN;  /* intentional gap */
            continue;
        }
        double d;
        if (fastchart_zval_to_double(v, &d) == 0) {
            out[n++] = d;
        } else if (strict) {
            zend_type_error("FastChart strict mode: series data must be numeric or null; got %s",
                zend_zval_type_name(v));
            return -1;
        } else {
            out[n++] = NAN;
        }
    } ZEND_HASH_FOREACH_END();
    return n;
}

/* Find data min and max across (a subset of) series. `right_axis`
 * picks which series to include: false includes left-axis series,
 * true includes only series with `right_axis = true`. Returns 0 if
 * at least one numeric value was seen, -1 if none or strict failed. */
static int compute_y_range(fastchart_line_series *series, int n_series,
                           bool strict, bool want_right,
                           double *out_min, double *out_max)
{
    int seen = 0;
    double mn = 0, mx = 0;
    for (int s = 0; s < n_series; s++) {
        if (series[s].right_axis != want_right) continue;
        zval *v;
        ZEND_HASH_FOREACH_VAL(series[s].data, v) {
            if (Z_TYPE_P(v) == IS_NULL) continue;  /* gap */
            double d;
            if (fastchart_zval_to_double(v, &d) != 0) {
                if (strict) {
                    zend_type_error("FastChart strict mode: series data must be numeric or null");
                    return -1;
                }
                continue;
            }
            if (!seen) { mn = mx = d; seen = 1; }
            else { if (d < mn) mn = d; if (d > mx) mx = d; }
        } ZEND_HASH_FOREACH_END();
    }
    if (!seen) return -1;
    *out_min = mn;
    *out_max = mx;
    return 0;
}

#define MAX_SERIES 8

int fastchart_line_render_to_image(fastchart_obj *self, gdImagePtr im)
{
    fastchart_line_series series[MAX_SERIES];
    int n_series = 0, max_len = 0;
    if (collect_line_series(&self->data, series, MAX_SERIES,
                            &n_series, &max_len) != 0 || n_series == 0) {
        zend_throw_error(NULL,
            "FastChart\\LineChart::draw() requires setSeries() to have been called with non-empty data");
        return -1;
    }

    /* Compute left + right Y ranges. With secondary axis on, series
     * route by the 'axis' key; without it, all series go to left. */
    int n_left = 0, n_right = 0;
    for (int s = 0; s < n_series; s++) {
        if (self->secondary_y && series[s].right_axis) n_right++;
        else { series[s].right_axis = false; n_left++; }
    }

    double dmin_l = 0, dmax_l = 0, dmin_r = 0, dmax_r = 0;
    if (n_left == 0) {
        zend_throw_error(NULL,
            "FastChart\\LineChart::draw(): primary Y axis has no series");
        return -1;
    }
    if (compute_y_range(series, n_series, self->strict, false, &dmin_l, &dmax_l) != 0) {
        if (!EG(exception)) {
            zend_throw_error(NULL,
                "FastChart\\LineChart::draw() found no numeric values for the primary Y axis");
        }
        return -1;
    }
    if (n_right > 0) {
        if (compute_y_range(series, n_series, self->strict, true, &dmin_r, &dmax_r) != 0) {
            if (!EG(exception)) {
                zend_throw_error(NULL,
                    "FastChart\\LineChart::draw() found no numeric values for the secondary Y axis");
            }
            return -1;
        }
    }

    fastchart_value_range range_l, range_r;
    if (self->y_axis_scale == FASTCHART_SCALE_LOG) {
        if (fastchart_value_range_compute_log(dmin_l, dmax_l, &range_l) != 0) {
            zend_value_error("FastChart\\LineChart::draw(): log Y-axis requires strictly-positive data");
            return -1;
        }
    } else {
        fastchart_value_range_compute(dmin_l, dmax_l, 6, &range_l);
        fastchart_value_range_apply_override(self, &range_l);
    }
    if (n_right > 0) {
        fastchart_value_range_compute(dmin_r, dmax_r, 6, &range_r);
    }

    fastchart_rect plot;
    fastchart_compute_layout(self, im, 1, 1, &plot);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, self, &pal);

    fastchart_draw_frame(im, self, &plot, &pal);
    fastchart_draw_title(im, self, &plot, &pal);
    fastchart_draw_y_axis(im, self, &plot, &pal, &range_l);
    if (n_right > 0) {
        fastchart_draw_y_axis_right(im, self, &plot, &pal, &range_r);
    }

    /* Category labels: borrowed pointers into still-rooted PHP
     * zend_strings, valid for the rest of this call. */
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

    int marker_style = self->marker_style >= 0
        ? (int)self->marker_style
        : FASTCHART_MARKER_CIRCLE;
    int marker_size = self->marker_size >= 1
        ? (int)self->marker_size
        : 6;

    int legend_colors[MAX_SERIES];
    const char *legend_labels[MAX_SERIES];
    int legend_count = 0;

    /* Optional per-point error bars (parallel to the first series). */
    HashTable *err_ht = NULL;
    {
        zval *eb = zend_hash_str_find(Z_ARRVAL(self->config),
            "error_bars", sizeof("error_bars") - 1);
        if (eb && Z_TYPE_P(eb) == IS_ARRAY) err_ht = Z_ARRVAL_P(eb);
    }

    double values[2048];
    fastchart_pt pts[2048];
    for (int s = 0; s < n_series; s++) {
        int color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
        const fastchart_value_range *rng =
            series[s].right_axis ? &range_r : &range_l;
        int n = extract_doubles(series[s].data, values,
                                series[s].len < 2048 ? series[s].len : 2048,
                                self->strict);
        if (n < 0) return -1;
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

        /* Error bars on the first (left-axis) series only -- multi-series
         * line charts get crowded fast otherwise. */
        if (err_ht && s == 0 && !series[s].right_axis) {
            for (int i = 0; i < n; i++) {
                if (!pts[i].valid) continue;
                zval *ev = zend_hash_index_find(err_ht, i);
                if (!ev) continue;
                double lo = 0, hi = 0;
                if (Z_TYPE_P(ev) == IS_ARRAY) {
                    zval *zlo = zend_hash_index_find(Z_ARRVAL_P(ev), 0);
                    zval *zhi = zend_hash_index_find(Z_ARRVAL_P(ev), 1);
                    if (zlo && zhi) {
                        fastchart_zval_to_double(zlo, &lo);
                        fastchart_zval_to_double(zhi, &hi);
                    }
                } else {
                    double m;
                    if (fastchart_zval_to_double(ev, &m) == 0 && m >= 0) lo = hi = m;
                }
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

        /* Polyline (linear or smooth depending on chart->line_interpolation). */
        fastchart_draw_polyline(im, self, pts, n, color, 2, true);

        /* Markers + value labels at each valid point. */
        for (int i = 0; i < n; i++) {
            if (!pts[i].valid) continue;
            int marker_color = color;
            if (series[s].colors) {
                zval *cv = zend_hash_index_find(series[s].colors, i);
                if (cv && Z_TYPE_P(cv) == IS_LONG) {
                    long c = Z_LVAL_P(cv);
                    if (c >= 0 && c <= 0xFFFFFF) {
                        marker_color = gdImageColorAllocate(im,
                            (int)((c >> 16) & 0xFF),
                            (int)((c >>  8) & 0xFF),
                            (int)( c        & 0xFF));
                    }
                }
            }
            fastchart_draw_marker(im, pts[i].x, pts[i].y,
                                  marker_style, marker_size, marker_color);
            fastchart_draw_value_label(im, self, &pal,
                                       pts[i].x, pts[i].y, values[i]);
        }

        if (series[s].label) {
            legend_colors[legend_count] = color;
            legend_labels[legend_count] = series[s].label;
            legend_count++;
        }
    }

    /* Combo overlays go on top of the primary data. */
    fastchart_draw_overlays_categorical(im, self, &plot, &pal,
                                         &range_l,
                                         n_right > 0 ? &range_r : NULL,
                                         max_len);

    fastchart_draw_h_annotations(im, self, &plot, &pal, &range_l);
    fastchart_draw_v_annotations_categorical(im, self, &plot, &pal, max_len);

    if (legend_count >= 1 && n_series >= 2) {
        fastchart_draw_legend(im, self, &plot, &pal,
                              legend_count, legend_colors, legend_labels);
    }

    fastchart_draw_text_annotations(im, self, &pal);

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

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (fastchart_line_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
