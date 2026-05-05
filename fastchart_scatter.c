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
#include "fastchart_axis.h"
#include "fastchart_text.h"

#define MAX_POINTS 8192
#define MAX_SCATTER_SERIES 8

/* Two accepted shapes:
 *   1. List of [x, y] pairs:           [[1.0, 2.5], [2.0, 3.1], ...]
 *   2. List of series with 'data' key: [{label, data: [[x,y], ...]}, ...]
 *
 * For v0.1 we keep the implementation linear in points and skip
 * non-numeric pairs silently rather than aborting the draw. */

typedef struct {
    double x, y;
    int series;  /* 0..7, indexes into pal.series */
} fastchart_scatter_point;

static int read_xy_pair(zval *pair, double *x, double *y)
{
    if (Z_TYPE_P(pair) != IS_ARRAY) return -1;
    HashTable *p = Z_ARRVAL_P(pair);
    if (zend_hash_num_elements(p) < 2) return -1;
    zval *zx = zend_hash_index_find(p, 0);
    zval *zy = zend_hash_index_find(p, 1);
    if (!zx || !zy) return -1;
    if (fastchart_zval_to_double(zx, x) != 0) return -1;
    if (fastchart_zval_to_double(zy, y) != 0) return -1;
    return 0;
}

/* `series_labels` is filled per series index encountered. Caller
 * passes a buffer of MAX_SCATTER_SERIES slots. Slots stay NULL for
 * series with no label, or when the input is single-series. */
static int collect_scatter_points(zval *data_zv,
                                  fastchart_scatter_point *out,
                                  int max_pts,
                                  int *out_n,
                                  const char **series_labels,
                                  int *out_n_series)
{
    *out_n = 0;
    *out_n_series = 0;
    if (Z_TYPE_P(data_zv) != IS_ARRAY) return -1;
    HashTable *ht = Z_ARRVAL_P(data_zv);
    if (zend_hash_num_elements(ht) == 0) return 0;

    /* Detect multi-series shape: first element is a dict with 'data'. */
    zval *first = zend_hash_index_find(ht, 0);
    int is_multi = 0;
    if (first && Z_TYPE_P(first) == IS_ARRAY) {
        zval *data_key = zend_hash_str_find(Z_ARRVAL_P(first),
                                            "data", sizeof("data") - 1);
        if (data_key && Z_TYPE_P(data_key) == IS_ARRAY) is_multi = 1;
    }

    if (is_multi) {
        zval *series_zv;
        int s = 0;
        ZEND_HASH_FOREACH_VAL(ht, series_zv) {
            if (Z_TYPE_P(series_zv) != IS_ARRAY) continue;
            if (s >= MAX_SCATTER_SERIES) break;
            zval *data_key = zend_hash_str_find(Z_ARRVAL_P(series_zv),
                                                "data", sizeof("data") - 1);
            if (!data_key || Z_TYPE_P(data_key) != IS_ARRAY) continue;

            zval *label_zv = zend_hash_str_find(Z_ARRVAL_P(series_zv),
                                                "label", sizeof("label") - 1);
            series_labels[s] = (label_zv && Z_TYPE_P(label_zv) == IS_STRING)
                ? Z_STRVAL_P(label_zv) : NULL;

            zval *pair;
            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(data_key), pair) {
                if (*out_n >= max_pts) goto done;
                double x, y;
                if (read_xy_pair(pair, &x, &y) != 0) continue;
                out[*out_n].x = x;
                out[*out_n].y = y;
                out[*out_n].series = s;
                (*out_n)++;
            } ZEND_HASH_FOREACH_END();
            s++;
        } ZEND_HASH_FOREACH_END();
        *out_n_series = s;
    } else {
        zval *pair;
        ZEND_HASH_FOREACH_VAL(ht, pair) {
            if (*out_n >= max_pts) break;
            double x, y;
            if (read_xy_pair(pair, &x, &y) != 0) continue;
            out[*out_n].x = x;
            out[*out_n].y = y;
            out[*out_n].series = 0;
            (*out_n)++;
        } ZEND_HASH_FOREACH_END();
        *out_n_series = 1;
    }

done:
    return 0;
}

int fastchart_scatter_render_to_image(fastchart_obj *self, gdImagePtr im)
{
    /* Per-call scratch. The previous file-static buffer made render
     * non-reentrant — broken under ZTS or any SAPI that runs two
     * draws concurrently. ~256 KB at MAX_POINTS=8192. */
    fastchart_scatter_point *points = ecalloc(MAX_POINTS, sizeof(*points));
    const char *series_labels[MAX_SCATTER_SERIES] = { NULL };
    int n = 0, n_series = 0;
    if (collect_scatter_points(&self->data, points, MAX_POINTS, &n,
                               series_labels, &n_series) != 0 || n == 0) {
        efree(points);
        zend_throw_error(NULL,
            "FastChart\\ScatterChart::draw() requires setPoints() with one or more [x, y] pairs");
        return -1;
    }

    /* Y range from data. */
    double y_min = points[0].y, y_max = points[0].y;
    double x_min = points[0].x, x_max = points[0].x;
    for (int i = 1; i < n; i++) {
        if (points[i].y < y_min) y_min = points[i].y;
        if (points[i].y > y_max) y_max = points[i].y;
        if (points[i].x < x_min) x_min = points[i].x;
        if (points[i].x > x_max) x_max = points[i].x;
    }

    fastchart_value_range yrange;
    if (self->y_axis_scale == FASTCHART_SCALE_LOG) {
        if (fastchart_value_range_compute_log(y_min, y_max, &yrange) != 0) {
            efree(points);
            zend_value_error("FastChart\\ScatterChart::draw(): log Y-axis requires strictly-positive Y values");
            return -1;
        }
    } else {
        fastchart_value_range_compute(y_min, y_max, 6, &yrange);
        fastchart_value_range_apply_override(self, &yrange);
    }

    /* X range. */
    fastchart_value_range xrange;
    fastchart_value_range_compute(x_min, x_max, 6, &xrange);

    fastchart_rect plot;
    fastchart_compute_layout(self, im, 1, 1, &plot);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, self, &pal);

    fastchart_draw_frame(im, self, &plot, &pal);
    fastchart_draw_title(im, self, &plot, &pal);
    fastchart_draw_y_axis(im, self, &plot, &pal, &yrange);

    /* Custom X axis: continuous numeric ticks from xrange. Reuse the
     * categorical axis line + tick infrastructure by drawing the line
     * and emitting ticks at xrange values. */
    gdImageLine(im, plot.x0, plot.y1, plot.x1, plot.y1, pal.axis);
    const char *font = xrange.n_ticks > 0
        ? fastchart_resolve_font(self, FC_FONT_AXIS) : NULL;
    if (font) {
        double size = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
        int label_y = plot.y1 + 4 + (int)(size * 1.2);
        for (int t = 0; t < xrange.n_ticks; t++) {
            double v = xrange.ticks[t];
            double frac = (v - xrange.min) / (xrange.max - xrange.min);
            int x = plot.x0 + (int)(frac * (plot.x1 - plot.x0) + 0.5);
            gdImageLine(im, x, plot.y1 + 1, x, plot.y1 + 4, pal.axis);

            char buf[32];
            int decimals = xrange.tick_step >= 1.0 ? 0
                : (int)ceil(-log10(xrange.tick_step));
            if (decimals < 0) decimals = 0;
            if (decimals > 4) decimals = 4;
            snprintf(buf, sizeof(buf), "%.*f", decimals, v);

            fastchart_text_draw(im, font, size, pal.text,
                                x, label_y, FASTCHART_ALIGN_CENTER,
                                buf, NULL, 0);
        }
    }

    fastchart_draw_axis_titles(im, self, &plot, &pal);

    /* Marker resolution: ScatterChart's default is a 7px circle. */
    int marker_style = self->marker_style >= 0
        ? (int)self->marker_style
        : FASTCHART_MARKER_CIRCLE;
    int marker_size = self->marker_size >= 1
        ? (int)self->marker_size
        : 7;

    /* Optional per-point error bars (parallel to setPoints index order). */
    HashTable *err_ht = NULL;
    {
        zval *eb = zend_hash_str_find(Z_ARRVAL(self->config),
            "error_bars", sizeof("error_bars") - 1);
        if (eb && Z_TYPE_P(eb) == IS_ARRAY) err_ht = Z_ARRVAL_P(eb);
    }

    for (int i = 0; i < n; i++) {
        double frac_x = (xrange.max - xrange.min) > 0
            ? (points[i].x - xrange.min) / (xrange.max - xrange.min)
            : 0.5;
        int px = plot.x0 + (int)(frac_x * (plot.x1 - plot.x0) + 0.5);
        int py = fastchart_y_to_pixel(points[i].y, &yrange, &plot);

        int color = pal.series[points[i].series % FASTCHART_PALETTE_SERIES_N];

        /* Error bar before marker so the marker overdraws the stem
         * cleanly. err_ht entries: scalar -> symmetric ±, [lo, hi] ->
         * asymmetric. */
        if (err_ht) {
            zval *ev = zend_hash_index_find(err_ht, i);
            if (ev) {
                double lo = 0, hi = 0;
                if (Z_TYPE_P(ev) == IS_ARRAY) {
                    zval *zlo = zend_hash_index_find(Z_ARRVAL_P(ev), 0);
                    zval *zhi = zend_hash_index_find(Z_ARRVAL_P(ev), 1);
                    if (zlo && zhi &&
                        fastchart_zval_to_double(zlo, &lo) == 0 &&
                        fastchart_zval_to_double(zhi, &hi) == 0) {
                        /* asymmetric: lo / hi as separate magnitudes */
                    } else { lo = hi = 0; }
                } else {
                    double m;
                    if (fastchart_zval_to_double(ev, &m) == 0 && m >= 0) {
                        lo = hi = m;
                    }
                }
                if (lo > 0 || hi > 0) {
                    int py_lo = fastchart_y_to_pixel(points[i].y - lo, &yrange, &plot);
                    int py_hi = fastchart_y_to_pixel(points[i].y + hi, &yrange, &plot);
                    gdImageLine(im, px, py_hi, px, py_lo, pal.axis);
                    gdImageLine(im, px - 4, py_hi, px + 4, py_hi, pal.axis);
                    gdImageLine(im, px - 4, py_lo, px + 4, py_lo, pal.axis);
                }
            }
        }

        fastchart_draw_marker(im, px, py, marker_style, marker_size, color);
        fastchart_draw_value_label(im, self, &pal, px, py, points[i].y);
    }

    /* Trend line: least-squares fit. Linear (degree=1) uses the
     * closed-form 2x2 solution; polynomial (degree>=2) builds the
     * normal-equations matrix and solves via Gaussian elimination.
     * The fitted curve is rendered as 200 sub-segments across the
     * x range. */
    if (self->trend_line && n >= 2) {
        int deg = (int)self->trend_degree;
        if (deg < 1) deg = 1;
        /* Cap at 3. Quintic / quartic fits over raw scatter data
         * are essentially never the right answer — they overfit
         * noise and the high-order Vandermonde is numerically
         * fragile even with normalization. Three is high enough
         * for the vast majority of "is there a trend?" use cases. */
        if (deg > 3) deg = 3;
        if (deg + 1 > n) deg = n - 1;
        if (deg < 1) deg = 1;

        int color = self->trend_line_color >= 0
            ? gdImageColorAllocate(im,
                (int)((self->trend_line_color >> 16) & 0xFF),
                (int)((self->trend_line_color >>  8) & 0xFF),
                (int)( self->trend_line_color        & 0xFF))
            : pal.axis;

        /* Normalize x to xn = (x - x_mid) / x_half so xn ∈ [-1, 1]
         * across the input range. The Vandermonde of normalized x
         * is several orders of magnitude better-conditioned than
         * raw x for any non-trivial domain (e.g. timestamps near
         * 1.7e9 + degree 3 produces matrix entries near 1e30, well
         * past double-precision recovery). Evaluation re-applies
         * the same normalization. */
        double x_mid  = 0.5 * (x_min + x_max);
        double x_half = 0.5 * (x_max - x_min);
        if (x_half <= 0) x_half = 1.0;

        double coeffs[4] = {0};
        if (deg == 1) {
            double sx = 0, sy = 0, sxx = 0, sxy = 0;
            for (int i = 0; i < n; i++) {
                double xn = (points[i].x - x_mid) / x_half;
                sx  += xn;
                sy  += points[i].y;
                sxx += xn * xn;
                sxy += xn * points[i].y;
            }
            double denom = n * sxx - sx * sx;
            if (denom != 0.0) {
                coeffs[1] = (n * sxy - sx * sy) / denom;
                coeffs[0] = (sy - coeffs[1] * sx) / n;
            }
        } else {
            /* Normal equations for polynomial of degree `deg` in
             * normalized x:
             *   A[k][j] = sum xn^(j+k)        for j,k in 0..deg
             *   b[k]    = sum y * xn^k
             * Solve A * c = b (size deg+1) via partial-pivot Gauss. */
            int m = deg + 1;
            double A[4][5] = {{0}};   /* augmented [m | b] */
            for (int i = 0; i < n; i++) {
                double xn = (points[i].x - x_mid) / x_half;
                double yi = points[i].y;
                double xpow_row[8]; /* xn^0 .. xn^(2*deg) for deg<=3 */
                xpow_row[0] = 1.0;
                for (int p = 1; p <= 2 * deg; p++) xpow_row[p] = xpow_row[p-1] * xn;
                for (int k = 0; k < m; k++) {
                    for (int j = 0; j < m; j++) {
                        A[k][j] += xpow_row[j + k];
                    }
                    A[k][m] += yi * xpow_row[k];
                }
            }
            /* Gauss-Jordan with partial pivoting. */
            for (int k = 0; k < m; k++) {
                int piv = k;
                double best = fabs(A[k][k]);
                for (int r = k + 1; r < m; r++) {
                    if (fabs(A[r][k]) > best) { best = fabs(A[r][k]); piv = r; }
                }
                if (best < 1e-12) { /* singular -- skip */ goto no_fit; }
                if (piv != k) {
                    for (int c = 0; c <= m; c++) {
                        double tmp = A[k][c]; A[k][c] = A[piv][c]; A[piv][c] = tmp;
                    }
                }
                double pivval = A[k][k];
                for (int c = 0; c <= m; c++) A[k][c] /= pivval;
                for (int r = 0; r < m; r++) {
                    if (r == k) continue;
                    double f = A[r][k];
                    if (f == 0) continue;
                    for (int c = 0; c <= m; c++) A[r][c] -= f * A[k][c];
                }
            }
            for (int k = 0; k < m; k++) coeffs[k] = A[k][m];
        }

        /* Plot 200 sub-segments. Normalize x exactly as the fit did. */
        const int N = 200;
        gdImageSetThickness(im, 2);
        int prev_px = 0, prev_py = 0;
        for (int s = 0; s <= N; s++) {
            double t = (double)s / (double)N;
            double x = x_min + t * (x_max - x_min);
            double xn = (x - x_mid) / x_half;
            double y = 0;
            double xp = 1.0;
            for (int k = 0; k <= deg; k++) { y += coeffs[k] * xp; xp *= xn; }
            double frac = (xrange.max - xrange.min) > 0
                ? (x - xrange.min) / (xrange.max - xrange.min) : 0.5;
            int px = plot.x0 + (int)(frac * (plot.x1 - plot.x0) + 0.5);
            int py = fastchart_y_to_pixel(y, &yrange, &plot);
            if (s > 0) gdImageLine(im, prev_px, prev_py, px, py, color);
            prev_px = px; prev_py = py;
        }
        gdImageSetThickness(im, 1);
        no_fit: ;
    }

    fastchart_draw_h_annotations(im, self, &plot, &pal, &yrange);
    fastchart_draw_v_annotations_continuous(im, self, &plot, &pal, &xrange);

    if (n_series >= 2) {
        int legend_colors[MAX_SCATTER_SERIES];
        const char *legend_labels[MAX_SCATTER_SERIES];
        int legend_count = 0;
        for (int s = 0; s < n_series; s++) {
            if (!series_labels[s]) continue;
            legend_colors[legend_count] = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            legend_labels[legend_count] = series_labels[s];
            legend_count++;
        }
        if (legend_count > 0) {
            fastchart_draw_legend(im, self, &plot, &pal,
                                  legend_count, legend_colors, legend_labels);
        }
    }

    fastchart_draw_text_annotations(im, self, &pal);

    /* Build the image-map area list for points that carry an
     * 'href' / 'tooltip' key. We re-walk the source data because
     * the flat points[] struct doesn't preserve the originating
     * dict. The map is parked on config["image_map_areas"] for
     * later retrieval via Chart::getImageMap(). */
    {
        zval map_list;
        array_init(&map_list);
        if (Z_TYPE(self->data) == IS_ARRAY) {
            int point_idx = 0;
            zval *first = zend_hash_index_find(Z_ARRVAL(self->data), 0);
            int multi = first && Z_TYPE_P(first) == IS_ARRAY &&
                zend_hash_str_find(Z_ARRVAL_P(first), "data", sizeof("data") - 1) != NULL;

            zval *outer;
            ZEND_HASH_FOREACH_VAL(Z_ARRVAL(self->data), outer) {
                if (Z_TYPE_P(outer) != IS_ARRAY) continue;
                HashTable *src;
                if (multi) {
                    zval *d = zend_hash_str_find(Z_ARRVAL_P(outer), "data", sizeof("data") - 1);
                    if (!d || Z_TYPE_P(d) != IS_ARRAY) continue;
                    src = Z_ARRVAL_P(d);
                } else {
                    src = NULL;
                }

                /* Single-series flat: outer is the point dict itself. */
                if (!multi) {
                    zval *href = zend_hash_str_find(Z_ARRVAL_P(outer), "href", sizeof("href") - 1);
                    if (href && Z_TYPE_P(href) == IS_STRING && point_idx < n) {
                        double frac_x = (xrange.max - xrange.min) > 0
                            ? (points[point_idx].x - xrange.min) / (xrange.max - xrange.min)
                            : 0.5;
                        int px = plot.x0 + (int)(frac_x * (plot.x1 - plot.x0) + 0.5);
                        int py = fastchart_y_to_pixel(points[point_idx].y, &yrange, &plot);
                        zval e;
                        array_init(&e);
                        add_assoc_long(&e, "x", px);
                        add_assoc_long(&e, "y", py);
                        add_assoc_long(&e, "r", marker_size);
                        add_assoc_str(&e, "href", zend_string_copy(Z_STR_P(href)));
                        zval *tip = zend_hash_str_find(Z_ARRVAL_P(outer), "tooltip", sizeof("tooltip") - 1);
                        if (tip && Z_TYPE_P(tip) == IS_STRING) {
                            add_assoc_str(&e, "title", zend_string_copy(Z_STR_P(tip)));
                        }
                        add_next_index_zval(&map_list, &e);
                    }
                    point_idx++;
                } else if (src) {
                    zval *p;
                    ZEND_HASH_FOREACH_VAL(src, p) {
                        if (Z_TYPE_P(p) == IS_ARRAY && point_idx < n) {
                            zval *href = zend_hash_str_find(Z_ARRVAL_P(p), "href", sizeof("href") - 1);
                            if (href && Z_TYPE_P(href) == IS_STRING) {
                                double frac_x = (xrange.max - xrange.min) > 0
                                    ? (points[point_idx].x - xrange.min) / (xrange.max - xrange.min)
                                    : 0.5;
                                int px = plot.x0 + (int)(frac_x * (plot.x1 - plot.x0) + 0.5);
                                int py = fastchart_y_to_pixel(points[point_idx].y, &yrange, &plot);
                                zval e;
                                array_init(&e);
                                add_assoc_long(&e, "x", px);
                                add_assoc_long(&e, "y", py);
                                add_assoc_long(&e, "r", marker_size);
                                add_assoc_str(&e, "href", zend_string_copy(Z_STR_P(href)));
                                zval *tip = zend_hash_str_find(Z_ARRVAL_P(p), "tooltip", sizeof("tooltip") - 1);
                                if (tip && Z_TYPE_P(tip) == IS_STRING) {
                                    add_assoc_str(&e, "title", zend_string_copy(Z_STR_P(tip)));
                                }
                                add_next_index_zval(&map_list, &e);
                            }
                        }
                        point_idx++;
                    } ZEND_HASH_FOREACH_END();
                }
            } ZEND_HASH_FOREACH_END();
        }
        zend_hash_str_update(Z_ARRVAL(self->config), "image_map_areas",
                             sizeof("image_map_areas") - 1, &map_list);
    }

    efree(points);
    return 0;
}

ZEND_METHOD(FastChart_ScatterChart, draw)
{
    zval *canvas_zv;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\ScatterChart::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (fastchart_scatter_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
