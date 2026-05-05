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

/* Build a 256-entry palette of interpolated colors from `from` -> `to`
 * via fastchart_lerp_rgb. One gdImageColorAllocate call per LUT entry,
 * once per gradient draw, so a 600-pixel-wide rectangle (or a 50k-pixel
 * polygon bbox) becomes ~256 allocs instead of 600 / 50k. */
static void build_gradient_lut(gdImagePtr im, int from, int to, int lut[256])
{
    for (int k = 0; k < 256; k++) {
        int rgb = fastchart_lerp_rgb(from, to, (double)k / 255.0);
        lut[k] = gdImageColorAllocate(im,
            (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
    }
}

static inline int lut_index(int pos, int span)
{
    if (span <= 0) return 0;
    int idx = (int)((double)pos / (double)span * 255.0 + 0.5);
    if (idx < 0) return 0;
    if (idx > 255) return 255;
    return idx;
}

int fastchart_gradient_filled_rectangle(gdImagePtr im, fastchart_obj *chart,
                                        int x0, int y0, int x1, int y1)
{
    if (!gradient_active(chart)) return 0;
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }

    int lut[256];
    build_gradient_lut(im, (int)chart->gradient_from, (int)chart->gradient_to, lut);

    if (chart->gradient_dir == FASTCHART_GRADIENT_HORIZONTAL) {
        int span = x1 - x0;
        for (int x = x0; x <= x1; x++) {
            gdImageLine(im, x, y0, x, y1, lut[lut_index(x - x0, span)]);
        }
    } else { /* vertical */
        int span = y1 - y0;
        for (int y = y0; y <= y1; y++) {
            gdImageLine(im, x0, y, x1, y, lut[lut_index(y - y0, span)]);
        }
    }
    return 1;
}

int fastchart_gradient_filled_polygon(gdImagePtr im, fastchart_obj *chart,
                                      gdPointPtr poly, int n_pts)
{
    if (!gradient_active(chart) || n_pts < 3) return 0;

    /* Render the polygon shape into a private mask image (paletted,
     * 1-bit) so we can read membership without colliding with any
     * RGB the user may already have on the destination canvas. Then
     * stamp the gradient color from a 256-entry LUT only where the
     * mask reads 1. This is the same idea the previous implementation
     * had, but without the can't-pick-a-safe-RGB-sentinel hazard:
     * the mask is its own image, completely isolated from `im`'s
     * existing pixels. */
    int xmin = poly[0].x, xmax = poly[0].x;
    int ymin = poly[0].y, ymax = poly[0].y;
    for (int i = 1; i < n_pts; i++) {
        if (poly[i].x < xmin) xmin = poly[i].x;
        if (poly[i].x > xmax) xmax = poly[i].x;
        if (poly[i].y < ymin) ymin = poly[i].y;
        if (poly[i].y > ymax) ymax = poly[i].y;
    }
    int mw = xmax - xmin + 1;
    int mh = ymax - ymin + 1;
    if (mw <= 0 || mh <= 0) return 1;

    gdImagePtr mask = gdImageCreate(mw, mh);
    if (!mask) return 0;
    int bg = gdImageColorAllocate(mask, 0, 0, 0);   /* outside */
    int fg = gdImageColorAllocate(mask, 255, 255, 255); /* inside */
    gdImageFilledRectangle(mask, 0, 0, mw - 1, mh - 1, bg);

    /* Translate the polygon into mask-local coords. */
    gdPoint *shifted = emalloc((size_t)n_pts * sizeof(gdPoint));
    for (int i = 0; i < n_pts; i++) {
        shifted[i].x = poly[i].x - xmin;
        shifted[i].y = poly[i].y - ymin;
    }
    gdImageFilledPolygon(mask, shifted, n_pts, fg);
    efree(shifted);

    int lut[256];
    build_gradient_lut(im, (int)chart->gradient_from, (int)chart->gradient_to, lut);

    if (chart->gradient_dir == FASTCHART_GRADIENT_HORIZONTAL) {
        int span = xmax - xmin;
        for (int x = xmin; x <= xmax; x++) {
            int c = lut[lut_index(x - xmin, span)];
            int mx = x - xmin;
            for (int y = ymin; y <= ymax; y++) {
                if (gdImageGetPixel(mask, mx, y - ymin) == fg) {
                    gdImageSetPixel(im, x, y, c);
                }
            }
        }
    } else {
        int span = ymax - ymin;
        for (int y = ymin; y <= ymax; y++) {
            int c = lut[lut_index(y - ymin, span)];
            int my = y - ymin;
            for (int x = xmin; x <= xmax; x++) {
                if (gdImageGetPixel(mask, x - xmin, my) == fg) {
                    gdImageSetPixel(im, x, y, c);
                }
            }
        }
    }
    gdImageDestroy(mask);
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
