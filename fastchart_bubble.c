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

#include <math.h>

/* Bubble: each entry is [x, y, size] or [x, y, size, rgb_color]. */
int fastchart_bubble_render_to_image(fastchart_obj *self, gdImagePtr im)
{
    HashTable *ht = Z_TYPE(self->data) == IS_ARRAY ? Z_ARRVAL(self->data) : NULL;
    if (!ht || zend_hash_num_elements(ht) == 0) {
        zend_throw_error(NULL,
            "FastChart\\BubbleChart::draw() requires setPoints() with non-empty data");
        return -1;
    }

    int n = (int)zend_hash_num_elements(ht);
    double *xs = ecalloc(n, sizeof(double));
    double *ys = ecalloc(n, sizeof(double));
    double *ss = ecalloc(n, sizeof(double));
    zend_long *cs = ecalloc(n, sizeof(zend_long));
    int    *has_color = ecalloc(n, sizeof(int));
    int collected = 0;

    zval *p;
    ZEND_HASH_FOREACH_VAL(ht, p) {
        if (Z_TYPE_P(p) != IS_ARRAY) continue;
        HashTable *t = Z_ARRVAL_P(p);
        zval *zx = zend_hash_index_find(t, 0);
        zval *zy = zend_hash_index_find(t, 1);
        zval *zs = zend_hash_index_find(t, 2);
        if (!zx || !zy || !zs) continue;
        double dx, dy, ds;
        if (fastchart_zval_to_double(zx, &dx) != 0) continue;
        if (fastchart_zval_to_double(zy, &dy) != 0) continue;
        if (fastchart_zval_to_double(zs, &ds) != 0) continue;
        if (ds < 0) ds = 0;
        xs[collected] = dx;
        ys[collected] = dy;
        ss[collected] = ds;
        zval *zc = zend_hash_index_find(t, 3);
        if (zc && Z_TYPE_P(zc) == IS_LONG && Z_LVAL_P(zc) >= 0 && Z_LVAL_P(zc) <= 0xFFFFFF) {
            cs[collected] = Z_LVAL_P(zc);
            has_color[collected] = 1;
        }
        collected++;
    } ZEND_HASH_FOREACH_END();

    if (collected == 0) {
        efree(xs); efree(ys); efree(ss); efree(cs); efree(has_color);
        zend_throw_error(NULL,
            "FastChart\\BubbleChart::draw() found no [x, y, size] points");
        return -1;
    }

    /* Compute axis ranges. */
    double xmin = xs[0], xmax = xs[0], ymin = ys[0], ymax = ys[0];
    double smax = ss[0];
    for (int i = 1; i < collected; i++) {
        if (xs[i] < xmin) xmin = xs[i];
        if (xs[i] > xmax) xmax = xs[i];
        if (ys[i] < ymin) ymin = ys[i];
        if (ys[i] > ymax) ymax = ys[i];
        if (ss[i] > smax) smax = ss[i];
    }

    fastchart_value_range yrange;
    fastchart_value_range_compute(ymin, ymax, 6, &yrange);
    fastchart_value_range_apply_override(self, &yrange);

    fastchart_value_range xrange;
    fastchart_value_range_compute(xmin, xmax, 6, &xrange);

    fastchart_rect plot;
    fastchart_compute_layout(self, im, 1, 1, &plot);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, self, &pal);

    fastchart_draw_frame(im, self, &plot, &pal);
    fastchart_draw_title(im, self, &plot, &pal);
    fastchart_draw_y_axis(im, self, &plot, &pal, &yrange);

    /* Categorical x labels would mismatch the continuous data; draw a
     * lightweight numeric scale by reusing the time axis path with
     * synthetic timestamps... actually for a bubble chart, just draw
     * the X axis line and let the user supplement with axis title. */
    if (self->x_axis_visible) {
        gdImageLine(im, plot.x0, plot.y1, plot.x1, plot.y1, pal.axis);
    }
    fastchart_draw_axis_titles(im, self, &plot, &pal);

    /* Map size to radius: max bubble = ~5% of plot width. */
    double plot_w = plot.x1 - plot.x0;
    double r_max = plot_w * 0.05;
    if (r_max < 4) r_max = 4;

    /* Pre-resolve translucent fills for the palette so the per-bubble
     * loop avoids gdImageColorAllocateAlpha on every iteration. The
     * override-color path still allocates inline but typically hits
     * for a small number of distinct values. */
    int palette_alpha[FASTCHART_PALETTE_SERIES_N];
    for (int i = 0; i < FASTCHART_PALETTE_SERIES_N; i++) {
        int c = pal.series[i];
        palette_alpha[i] = gdImageColorAllocateAlpha(im,
            gdImageRed(im, c), gdImageGreen(im, c), gdImageBlue(im, c), 70);
    }

    gdImageAlphaBlending(im, 1);
    for (int i = 0; i < collected; i++) {
        double xfrac = (xrange.max - xrange.min) > 0
            ? (xs[i] - xrange.min) / (xrange.max - xrange.min) : 0.5;
        int px = plot.x0 + (int)(xfrac * (plot.x1 - plot.x0));
        int py = fastchart_y_to_pixel(ys[i], &yrange, &plot);
        double sfrac = smax > 0 ? sqrt(ss[i] / smax) : 0.5;
        int rad = (int)(r_max * sfrac);
        if (rad < 2) rad = 2;

        int color, alpha;
        if (has_color[i]) {
            color = gdImageColorAllocate(im,
                (cs[i] >> 16) & 0xFF, (cs[i] >> 8) & 0xFF, cs[i] & 0xFF);
            alpha = gdImageColorAllocateAlpha(im,
                (cs[i] >> 16) & 0xFF, (cs[i] >> 8) & 0xFF, cs[i] & 0xFF, 70);
        } else {
            int idx = i % FASTCHART_PALETTE_SERIES_N;
            color = pal.series[idx];
            alpha = palette_alpha[idx];
        }

        fastchart_shadow_filled_arc(im, self, px, py, rad * 2, 0, 360);
        gdImageFilledEllipse(im, px, py, rad * 2, rad * 2, alpha);
        int edge = self->edge_color >= 0 ? (int)self->edge_color : color;
        gdImageEllipse(im, px, py, rad * 2, rad * 2, edge);
    }
    gdImageAlphaBlending(im, 0);

    efree(xs); efree(ys); efree(ss); efree(cs); efree(has_color);

    fastchart_draw_h_annotations(im, self, &plot, &pal, &yrange);
    fastchart_draw_text_annotations(im, self, &pal);
    return 0;
}

ZEND_METHOD(FastChart_BubbleChart, draw)
{
    zval *canvas_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\BubbleChart::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (fastchart_bubble_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
