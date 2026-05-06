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
#include "fastchart_text.h"
#include "fastchart_effects.h"

#include <math.h>

/* Match the public setter cap so accepted data renders end-to-end.
 * 1024 gdPoint = 8 KB on the stack, well within budget. The previous
 * 512 silently dropped half of an at-cap series. */
#define MAX_POLAR_POINTS FASTCHART_MAX_POLAR_POINTS

int fastchart_polar_render_to_image(fastchart_polar_obj *self, gdImagePtr im)
{
    if (self->n_series == 0) {
        zend_throw_error(NULL,
            "FastChart\\PolarChart::draw() requires setSeries() with non-empty data");
        return -1;
    }
    fastchart_polar_series *series = self->series;
    int n_series = self->n_series;

    double rmax = 0;
    for (int s = 0; s < n_series; s++) {
        for (int i = 0; i < series[s].len; i++) {
            double r = series[s].radii[i];
            if (r > rmax) rmax = r;
        }
    }
    if (self->polar_max_radius > 0) rmax = self->polar_max_radius;
    if (rmax <= 0) rmax = 1.0;

    /* Per-render entry: invalidate the font cache (so a runtime
     * open_basedir narrowing between draws is honored) and stamp DPI
     * on the canvas. Must come BEFORE any palette / text work. */
    fastchart_begin_render((fastchart_obj *)self, im);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, (fastchart_obj *)self, &pal);

    int W = gdImageSX(im);
    int H = gdImageSY(im);
    gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, pal.bg);

    int top = (self->title && ZSTR_LEN(self->title) > 0) ? 32 : 8;
    int cx = W / 2;
    int cy = (top + H) / 2;
    int radius = (W < H ? W : H) / 2 - 40;
    if (radius < 40) radius = 40;

    /* Concentric grid + radial spokes every 30°. */
    const int rings = 4;
    for (int r = 1; r <= rings; r++) {
        int rr = (int)((double)radius * (double)r / (double)rings);
        gdImageEllipse(im, cx, cy, rr * 2, rr * 2, pal.grid);
    }
    for (int a = 0; a < 360; a += 30) {
        double rad = a * M_PI / 180.0;
        int tx = cx + (int)(radius * cos(rad));
        int ty = cy - (int)(radius * sin(rad));
        gdImageLine(im, cx, cy, tx, ty, pal.grid);
    }

    int legend_colors[FASTCHART_MAX_POLAR_SERIES];
    const char *legend_labels[FASTCHART_MAX_POLAR_SERIES];
    int legend_count = 0;

    for (int s = 0; s < n_series; s++) {
        int color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
        if (series[s].color_rgb >= 0) {
            int rgb = series[s].color_rgb;
            color = gdImageColorAllocate(im,
                (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
        }

        int upto = series[s].len < MAX_POLAR_POINTS ? series[s].len : MAX_POLAR_POINTS;

        /* STYLE_ROSE: each (angle, radius) becomes an angular wedge
         * extending from the centre out to `radius * r/rmax`. The
         * angular width is the gap to the next point's angle (or
         * 360/n if a single series is uniformly spaced). Wedges are
         * filled with the series colour and outlined in the border
         * colour for visual separation. The legacy line/area style
         * stays as the default branch below. */
        if (self->polar_style == FASTCHART_POLAR_STYLE_ROSE) {
            if (upto < 1) continue;
            for (int i = 0; i < upto; i++) {
                double a0 = series[s].angles[i];
                double a1 = (i + 1 < upto)
                    ? series[s].angles[i + 1]
                    : a0 + 360.0 / (double)upto;
                double r = series[s].radii[i];
                if (r <= 0) continue;
                double rr_px = (double)radius * r / rmax;
                /* libgd arc angles are clockwise with 0° at 3-o'clock;
                 * our angles are CCW math angles. Convert by negating
                 * and adding 360 so libgd renders the wedge oriented
                 * the same way as the line/area branch. */
                int gd_a = (int)((360.0 - a1)) % 360;
                int gd_b = (int)((360.0 - a0)) % 360;
                if (gd_a < 0) gd_a += 360;
                if (gd_b < 0) gd_b += 360;
                if (gd_b <= gd_a) gd_b += 360;
                gdImageFilledArc(im, cx, cy, (int)(rr_px * 2), (int)(rr_px * 2),
                                 gd_a, gd_b, color, gdPie);
                gdImageArc(im, cx, cy, (int)(rr_px * 2), (int)(rr_px * 2),
                           gd_a, gd_b, pal.border);
            }
            if (series[s].label && legend_count < FASTCHART_MAX_POLAR_SERIES) {
                legend_colors[legend_count] = color;
                legend_labels[legend_count] = series[s].label;
                legend_count++;
            }
            continue;
        }

        gdPoint poly[MAX_POLAR_POINTS];
        int n_pts = 0;
        for (int i = 0; i < upto; i++) {
            double a = series[s].angles[i];
            double r = series[s].radii[i];
            double rad = a * M_PI / 180.0;
            double rr = radius * r / rmax;
            poly[n_pts].x = cx + (int)(rr * cos(rad));
            poly[n_pts].y = cy - (int)(rr * sin(rad));
            n_pts++;
        }
        if (n_pts < 2) continue;

        if (self->polar_filled && n_pts >= 3) {
            int rr = gdImageRed(im, color);
            int gg = gdImageGreen(im, color);
            int bb = gdImageBlue(im, color);
            int alpha = gdImageColorAllocateAlpha(im, rr, gg, bb, 90);
            gdImageAlphaBlending(im, 1);
            fastchart_shadow_filled_polygon(im, (fastchart_obj *)self, poly, n_pts);
            fastchart_filled_polygon_aa(im, poly, n_pts, alpha);
            gdImageAlphaBlending(im, 0);
        }
        gdImageSetThickness(im, 2);
        if (self->polar_filled && n_pts >= 3) {
            gdImagePolygon(im, poly, n_pts, color);
        } else {
            for (int i = 0; i < n_pts - 1; i++) {
                gdImageLine(im, poly[i].x, poly[i].y,
                            poly[i+1].x, poly[i+1].y, color);
            }
        }
        gdImageSetThickness(im, 1);
        for (int i = 0; i < n_pts; i++) {
            fastchart_draw_marker(im, poly[i].x, poly[i].y,
                                  FASTCHART_MARKER_CIRCLE, 5, color);
        }
        if (series[s].label && legend_count < FASTCHART_MAX_POLAR_SERIES) {
            legend_colors[legend_count] = color;
            legend_labels[legend_count] = series[s].label;
            legend_count++;
        }
    }

    /* Title. */
    fastchart_draw_floating_title(im, (fastchart_obj *)self, &pal, W / 2, 24);

    if (legend_count > 0) {
        fastchart_rect plot = { 10, top, W - 10, H - 10 };
        fastchart_draw_legend(im, (fastchart_obj *)self, &plot, &pal,
                              legend_count, legend_colors, legend_labels);
    }

    fastchart_draw_text_annotations(im, (fastchart_obj *)self, &pal);
    return 0;
}

ZEND_METHOD(FastChart_PolarChart, draw)
{
    zval *canvas_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();
    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\PolarChart::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }
    if (!fastchart_require_truecolor(im)) RETURN_THROWS();
    fastchart_polar_obj *self = Z_FASTCHART_POLAR_OBJ_P(ZEND_THIS);
    if (fastchart_polar_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
