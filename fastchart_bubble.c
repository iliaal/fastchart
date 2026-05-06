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
int fastchart_bubble_render_to_image(fastchart_bubble_obj *self, gdImagePtr im)
{
    if (self->point_count == 0) {
        zend_throw_error(NULL,
            "FastChart\\BubbleChart::draw() requires setPoints() with non-empty data");
        return -1;
    }
    fastchart_bubble_point *pts = self->points;
    int collected = self->point_count;

    double xmin = pts[0].x, xmax = pts[0].x, ymin = pts[0].y, ymax = pts[0].y;
    double smax = pts[0].size;
    for (int i = 1; i < collected; i++) {
        if (pts[i].x < xmin) xmin = pts[i].x;
        if (pts[i].x > xmax) xmax = pts[i].x;
        if (pts[i].y < ymin) ymin = pts[i].y;
        if (pts[i].y > ymax) ymax = pts[i].y;
        if (pts[i].size > smax) smax = pts[i].size;
    }

    fastchart_value_range yrange;
    fastchart_value_range_compute(ymin, ymax, 6, &yrange);
    fastchart_value_range_apply_override((fastchart_obj *)self, &yrange);

    fastchart_value_range xrange;
    fastchart_value_range_compute(xmin, xmax, 6, &xrange);

    fastchart_rect plot;
    fastchart_compute_layout((fastchart_obj *)self, im, 1, 1, NULL, 0, &plot);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, (fastchart_obj *)self, &pal);

    fastchart_color_cache color_cache;
    fastchart_color_cache_init(&color_cache);

    fastchart_draw_frame(im, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_title(im, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_y_axis(im, (fastchart_obj *)self, &plot, &pal, &yrange);
    fastchart_draw_plot_bands(im, (fastchart_obj *)self, &plot, &yrange, &pal);
    fastchart_draw_v_plot_bands_xrange(im, (fastchart_obj *)self, &plot,
                                       &xrange, &pal);

    /* Categorical x labels would mismatch the continuous data; draw a
     * lightweight numeric scale by reusing the time axis path with
     * synthetic timestamps... actually for a bubble chart, just draw
     * the X axis line and let the user supplement with axis title. */
    if (self->x_axis_visible) {
        gdImageLine(im, plot.x0, plot.y1, plot.x1, plot.y1, pal.axis);
    }
    fastchart_draw_axis_titles(im, (fastchart_obj *)self, &plot, &pal);

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
            ? (pts[i].x - xrange.min) / (xrange.max - xrange.min) : 0.5;
        int px = plot.x0 + (int)(xfrac * (plot.x1 - plot.x0));
        int py = fastchart_y_to_pixel(pts[i].y, &yrange, &plot);
        double sfrac = smax > 0 ? sqrt(pts[i].size / smax) : 0.5;
        int rad = (int)(r_max * sfrac);
        if (rad < 2) rad = 2;

        int color, alpha;
        if (pts[i].color_rgb >= 0) {
            int c = pts[i].color_rgb;
            color = fastchart_color_cache_get(&color_cache, im, c);
            /* Translucent variant of the same RGB. gdImage's truecolor
             * hash dedupes RGBA tuples, so the per-call cost is a hash
             * hit after the first unique color. */
            alpha = gdImageColorAllocateAlpha(im,
                (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, 70);
        } else {
            int idx = i % FASTCHART_PALETTE_SERIES_N;
            color = pal.series[idx];
            alpha = palette_alpha[idx];
        }

        fastchart_shadow_filled_arc(im, (fastchart_obj *)self, px, py, rad * 2, 0, 360);
        gdImageFilledEllipse(im, px, py, rad * 2, rad * 2, alpha);
        int edge = self->edge_color >= 0 ? (int)self->edge_color : color;
        gdImageEllipse(im, px, py, rad * 2, rad * 2, edge);
    }
    gdImageAlphaBlending(im, 0);

    fastchart_draw_h_annotations(im, (fastchart_obj *)self, &plot, &pal, &yrange);
    fastchart_draw_v_annotations_continuous(im, (fastchart_obj *)self, &plot, &pal, &xrange);
    fastchart_draw_text_annotations(im, (fastchart_obj *)self, &pal);

    if (self->icons && self->n_icons > 0) {
        for (int i = 0; i < self->n_icons; i++) {
            const fastchart_icon *ic = &self->icons[i];
            int px = fastchart_x_to_pixel(ic->x, &xrange, &plot);
            int py = fastchart_y_to_pixel(ic->y, &yrange, &plot);
            fastchart_blit_icon(im, ic, px, py);
        }
    }
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
    if (!fastchart_require_truecolor(im)) RETURN_THROWS();
    fastchart_bubble_obj *self = Z_FASTCHART_BUBBLE_OBJ_P(ZEND_THIS);
    if (fastchart_bubble_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
