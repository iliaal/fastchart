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
#include "fastchart_target.h"
#include "fastchart_axis.h"
#include "fastchart_effects.h"

#include <math.h>

/* Bubble: each entry is [x, y, size] or [x, y, size, rgb_color]. */
int fastchart_bubble_render_to_target(fastchart_bubble_obj *self, fastchart_target_t *t)
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

    /* Pad the data range so the largest bubble doesn't clip outside
     * the plot rect. Bubble radii are mapped to ~5% of plot width
     * (see r_max below), but the axis range is computed from raw
     * data y/x and the niced-tick rounding may leave only a few
     * pixels of headroom. A 10% relative pad on each end covers the
     * typical max-radius case and stays invisible when data span is
     * tiny (a flat-line set still rounds to a sensible niced range). */
    if (xmax > xmin) {
        double xpad = (xmax - xmin) * 0.10;
        xmin -= xpad;
        xmax += xpad;
    }
    if (ymax > ymin) {
        double ypad = (ymax - ymin) * 0.10;
        ymin -= ypad;
        ymax += ypad;
    }

    fastchart_value_range yrange;
    fastchart_value_range_compute(ymin, ymax, 6, &yrange);
    fastchart_value_range_apply_override((fastchart_obj *)self, &yrange);

    fastchart_value_range xrange;
    fastchart_value_range_compute(xmin, xmax, 6, &xrange);

    fastchart_rect plot;
    fastchart_compute_layout((fastchart_obj *)self, t, 1, 1, NULL, 0, &plot);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    fastchart_draw_frame(t, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_title(t, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_y_axis(t, (fastchart_obj *)self, &plot, &pal, &yrange);
    fastchart_draw_plot_bands(t, (fastchart_obj *)self, &plot, &yrange, &pal);
    fastchart_draw_v_plot_bands_xrange(t, (fastchart_obj *)self, &plot,
                                       &xrange, &pal);

    /* Categorical x labels would mismatch the continuous data; draw a
     * lightweight numeric scale by reusing the time axis path with
     * synthetic timestamps... actually for a bubble chart, just draw
     * the X axis line and let the user supplement with axis title. */
    if (self->x_axis_visible) {
        fastchart_target_line(t, plot.x0, plot.y1, plot.x1, plot.y1,
                              pal.axis, 1, FASTCHART_DASH_SOLID);
    }
    fastchart_draw_axis_titles(t, (fastchart_obj *)self, &plot, &pal);

    /* Map size to radius: max bubble = ~5% of plot width. */
    double plot_w = plot.x1 - plot.x0;
    double r_max = plot_w * 0.05;
    if (r_max < 4) r_max = 4;

    bool gd = (t->kind == FASTCHART_TARGET_GD);
    gdImagePtr im = gd ? t->u.gd.im : NULL;

    /* Pre-resolve translucent palette-series fills as target color
     * handles. gd_alpha 70 (~45% transparent) maps to alpha byte
     * 255 - 70*2 = 115 in the target API. */
    int palette_alpha[FASTCHART_PALETTE_SERIES_N];
    for (int i = 0; i < FASTCHART_PALETTE_SERIES_N; i++) {
        uint32_t rgba = fastchart_target_color_to_rgba(t, pal.series[i]);
        int r = (rgba >> 16) & 0xFF;
        int g = (rgba >>  8) & 0xFF;
        int b =  rgba        & 0xFF;
        palette_alpha[i] = fastchart_target_color(t, r, g, b, 115);
    }

    if (gd) gdImageAlphaBlending(im, 1);
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
            color = fastchart_target_color_rgb(t, c);
            alpha = fastchart_target_color(t,
                (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, 115);
        } else {
            int idx = i % FASTCHART_PALETTE_SERIES_N;
            color = pal.series[idx];
            alpha = palette_alpha[idx];
        }

        if (gd) {
            fastchart_shadow_filled_arc(im, (fastchart_obj *)self, px, py, rad * 2, 0, 360);
        }
        fastchart_target_ellipse(t, px, py, rad, rad, alpha, 1, 0);
        int edge = self->edge_color >= 0
            ? fastchart_target_color_rgb(t, (int)self->edge_color)
            : color;
        fastchart_target_ellipse(t, px, py, rad, rad, edge, 0, 1);
    }
    if (gd) gdImageAlphaBlending(im, 0);

    fastchart_draw_h_annotations(t, (fastchart_obj *)self, &plot, &pal, &yrange);
    fastchart_draw_v_annotations_continuous(t, (fastchart_obj *)self, &plot, &pal, &xrange);
    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);

    if (self->icons && self->n_icons > 0) {
        for (int i = 0; i < self->n_icons; i++) {
            const fastchart_icon *ic = &self->icons[i];
            int px = fastchart_x_to_pixel(ic->x, &xrange, &plot);
            int py = fastchart_y_to_pixel(ic->y, &yrange, &plot);
            fastchart_blit_icon(t, ic, px, py);
        }
    }
    return 0;
}

/* GD-only shim. */
int fastchart_bubble_render_to_image(fastchart_bubble_obj *self, gdImagePtr im)
{
    fastchart_target_t t;
    fastchart_target_from_gd(&t, im, self->dpi);
    return fastchart_bubble_render_to_target(self, &t);
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
