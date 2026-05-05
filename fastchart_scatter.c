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
#include "fastchart_text.h"

#define MAX_POINTS 8192

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

static int collect_scatter_points(zval *data_zv,
                                  fastchart_scatter_point *out,
                                  int max_pts,
                                  int *out_n)
{
    *out_n = 0;
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
            zval *data_key = zend_hash_str_find(Z_ARRVAL_P(series_zv),
                                                "data", sizeof("data") - 1);
            if (!data_key || Z_TYPE_P(data_key) != IS_ARRAY) continue;

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
    }

done:
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

    static fastchart_scatter_point points[MAX_POINTS];
    int n = 0;
    if (collect_scatter_points(&self->data, points, MAX_POINTS, &n) != 0 || n == 0) {
        zend_throw_error(NULL,
            "FastChart\\ScatterChart::draw() requires setPoints() with one or more [x, y] pairs");
        RETURN_THROWS();
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
    fastchart_value_range_compute(y_min, y_max, 6, &yrange);

    /* X range -- we use a value range too (similar nice-tick logic)
     * for label placement, but we plot positions linearly across the
     * range, not at categorical tick centers. */
    fastchart_value_range xrange;
    fastchart_value_range_compute(x_min, x_max, 6, &xrange);

    fastchart_rect plot;
    fastchart_compute_layout(self, im, 1, 1, &plot);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);

    fastchart_draw_frame(im, self, &plot, &pal);
    fastchart_draw_title(im, self, &plot, &pal);
    fastchart_draw_y_axis(im, self, &plot, &pal, &yrange);

    /* Custom X axis: continuous numeric ticks from xrange. Reuse the
     * categorical axis line + tick infrastructure by drawing the line
     * and emitting ticks at xrange values. */
    gdImageLine(im, plot.x0, plot.y1, plot.x1, plot.y1, pal.axis);
    if (self->font_path && xrange.n_ticks > 0) {
        double size = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
        const char *font = ZSTR_VAL(self->font_path);
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

    /* Plot the points. Filled disks; multi-series uses palette colors. */
    for (int i = 0; i < n; i++) {
        double frac_x = (xrange.max - xrange.min) > 0
            ? (points[i].x - xrange.min) / (xrange.max - xrange.min)
            : 0.5;
        int px = plot.x0 + (int)(frac_x * (plot.x1 - plot.x0) + 0.5);
        int py = fastchart_y_to_pixel(points[i].y, &yrange, &plot);

        int color = pal.series[points[i].series % FASTCHART_PALETTE_SERIES_N];
        gdImageFilledEllipse(im, px, py, 7, 7, color);
    }

    RETURN_ZVAL(canvas_zv, 1, 0);
}
