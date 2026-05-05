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
#include "fastchart_effects.h"
#include "fastchart_text.h"

#include <gd.h>

int fastchart_apply_line_style(gdImagePtr im, fastchart_obj *chart, int color)
{
    if (chart->line_style == FASTCHART_LINE_SOLID) return color;

    /* Repeating pattern of `color` then `gdTransparent`. Dotted has a
     * 1:1 ratio (pixel on, pixel off); dashed runs 5:3 for clearer
     * dashes at typical line widths. */
    int pattern[16];
    int len;
    if (chart->line_style == FASTCHART_LINE_DOTTED) {
        pattern[0] = color;
        pattern[1] = gdTransparent;
        len = 2;
    } else { /* DASHED */
        for (int i = 0; i < 5; i++) pattern[i] = color;
        for (int i = 5; i < 8; i++) pattern[i] = gdTransparent;
        len = 8;
    }
    gdImageSetStyle(im, pattern, len);
    return gdStyled;
}

int fastchart_lerp_rgb(int from, int to, double t)
{
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    int r0 = (from >> 16) & 0xFF, g0 = (from >> 8) & 0xFF, b0 = from & 0xFF;
    int r1 = (to   >> 16) & 0xFF, g1 = (to   >> 8) & 0xFF, b1 = to   & 0xFF;
    int r = (int)(r0 + (r1 - r0) * t + 0.5);
    int g = (int)(g0 + (g1 - g0) * t + 0.5);
    int b = (int)(b0 + (b1 - b0) * t + 0.5);
    return (r << 16) | (g << 8) | b;
}

static int gradient_active(const fastchart_obj *chart)
{
    return chart->gradient_from >= 0 && chart->gradient_to >= 0;
}

int fastchart_gradient_filled_rectangle(gdImagePtr im, fastchart_obj *chart,
                                        int x0, int y0, int x1, int y1,
                                        int solid_color)
{
    (void)solid_color;
    if (!gradient_active(chart)) return 0;
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }

    int from = (int)chart->gradient_from;
    int to   = (int)chart->gradient_to;

    if (chart->gradient_dir == FASTCHART_GRADIENT_HORIZONTAL) {
        int span = x1 - x0;
        for (int x = x0; x <= x1; x++) {
            double t = span > 0 ? (double)(x - x0) / (double)span : 0.0;
            int rgb = fastchart_lerp_rgb(from, to, t);
            int c = gdImageColorAllocate(im,
                (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
            gdImageLine(im, x, y0, x, y1, c);
        }
    } else { /* vertical */
        int span = y1 - y0;
        for (int y = y0; y <= y1; y++) {
            double t = span > 0 ? (double)(y - y0) / (double)span : 0.0;
            int rgb = fastchart_lerp_rgb(from, to, t);
            int c = gdImageColorAllocate(im,
                (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
            gdImageLine(im, x0, y, x1, y, c);
        }
    }
    return 1;
}

int fastchart_gradient_filled_polygon(gdImagePtr im, fastchart_obj *chart,
                                      gdPointPtr poly, int n_pts,
                                      int solid_color)
{
    if (!gradient_active(chart) || n_pts < 3) return 0;

    /* Find the bbox so we can paint scanlines in the gradient axis,
     * then use libgd to clip the row to the polygon shape via a
     * temporary mask trick: paint the polygon in `solid_color`, then
     * recolor each scanline with the interpolated gradient via
     * brute-force per-pixel comparison (~one pass, dominated by the
     * fill rect anyway). For typical chart shapes this is fast and
     * keeps the implementation small. */
    int xmin = poly[0].x, xmax = poly[0].x;
    int ymin = poly[0].y, ymax = poly[0].y;
    for (int i = 1; i < n_pts; i++) {
        if (poly[i].x < xmin) xmin = poly[i].x;
        if (poly[i].x > xmax) xmax = poly[i].x;
        if (poly[i].y < ymin) ymin = poly[i].y;
        if (poly[i].y > ymax) ymax = poly[i].y;
    }

    /* Sentinel solid color the polygon starts in -- pick a color
     * unlikely to clash with the gradient by allocating a fresh one. */
    int sentinel = gdImageColorAllocate(im, 1, 2, 3);
    gdImageFilledPolygon(im, poly, n_pts, sentinel);

    int from = (int)chart->gradient_from;
    int to   = (int)chart->gradient_to;
    int sentinel_idx = sentinel;

    if (chart->gradient_dir == FASTCHART_GRADIENT_HORIZONTAL) {
        int span = xmax - xmin;
        for (int x = xmin; x <= xmax; x++) {
            double t = span > 0 ? (double)(x - xmin) / (double)span : 0.0;
            int rgb = fastchart_lerp_rgb(from, to, t);
            int c = gdImageColorAllocate(im,
                (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
            for (int y = ymin; y <= ymax; y++) {
                if (gdImageGetPixel(im, x, y) == sentinel_idx) {
                    gdImageSetPixel(im, x, y, c);
                }
            }
        }
    } else {
        int span = ymax - ymin;
        for (int y = ymin; y <= ymax; y++) {
            double t = span > 0 ? (double)(y - ymin) / (double)span : 0.0;
            int rgb = fastchart_lerp_rgb(from, to, t);
            int c = gdImageColorAllocate(im,
                (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
            for (int x = xmin; x <= xmax; x++) {
                if (gdImageGetPixel(im, x, y) == sentinel_idx) {
                    gdImageSetPixel(im, x, y, c);
                }
            }
        }
    }
    (void)solid_color;
    return 1;
}

static int shadow_color_alloc(gdImagePtr im, fastchart_obj *chart)
{
    int r = (chart->shadow_color >> 16) & 0xFF;
    int g = (chart->shadow_color >>  8) & 0xFF;
    int b =  chart->shadow_color        & 0xFF;
    return gdImageColorAllocateAlpha(im, r, g, b, (int)chart->shadow_alpha);
}

void fastchart_shadow_filled_rectangle(gdImagePtr im, fastchart_obj *chart,
                                       int x0, int y0, int x1, int y1)
{
    if (!chart->has_drop_shadow) return;
    int c = shadow_color_alloc(im, chart);
    int dx = (int)chart->shadow_dx;
    int dy = (int)chart->shadow_dy;
    gdImageAlphaBlending(im, 1);
    gdImageFilledRectangle(im, x0 + dx, y0 + dy, x1 + dx, y1 + dy, c);
}

void fastchart_shadow_filled_polygon(gdImagePtr im, fastchart_obj *chart,
                                     gdPointPtr poly, int n_pts)
{
    if (!chart->has_drop_shadow || n_pts < 3) return;
    int c = shadow_color_alloc(im, chart);
    int dx = (int)chart->shadow_dx;
    int dy = (int)chart->shadow_dy;

    gdPoint *shifted = emalloc(n_pts * sizeof(gdPoint));
    for (int i = 0; i < n_pts; i++) {
        shifted[i].x = poly[i].x + dx;
        shifted[i].y = poly[i].y + dy;
    }
    gdImageAlphaBlending(im, 1);
    gdImageFilledPolygon(im, shifted, n_pts, c);
    efree(shifted);
}

void fastchart_shadow_filled_arc(gdImagePtr im, fastchart_obj *chart,
                                 int cx, int cy, int diameter,
                                 int start_deg, int end_deg)
{
    if (!chart->has_drop_shadow) return;
    int c = shadow_color_alloc(im, chart);
    int dx = (int)chart->shadow_dx;
    int dy = (int)chart->shadow_dy;
    gdImageAlphaBlending(im, 1);
    gdImageFilledArc(im, cx + dx, cy + dy, diameter, diameter,
                     start_deg, end_deg, c, gdPie);
}

void fastchart_shadow_text(gdImagePtr im, fastchart_obj *chart,
                           const char *font, double size,
                           int x, int y, double angle,
                           const char *text)
{
    if (!chart->has_drop_shadow || !font) return;
    int c = shadow_color_alloc(im, chart);
    int dx = (int)chart->shadow_dx;
    int dy = (int)chart->shadow_dy;
    gdImageAlphaBlending(im, 1);
    if (angle == 0.0) {
        fastchart_text_draw(im, font, size, c,
                            x + dx, y + dy, FASTCHART_ALIGN_CENTER,
                            text, NULL, 0);
    } else {
        fastchart_text_draw_rotated(im, font, size, c,
                                    x + dx, y + dy, FASTCHART_ALIGN_CENTER,
                                    angle, text, NULL, 0);
    }
}
