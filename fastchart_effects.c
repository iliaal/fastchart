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
#include "fastchart_effects.h"
#include "fastchart_target.h"
#include "fastchart_text.h"

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

int fastchart_gradient_filled_rectangle(fastchart_target_t *t,
                                        fastchart_obj *chart,
                                        int x0, int y0, int x1, int y1)
{
    if (!gradient_active(chart)) return 0;
    if (x1 < x0) { int tmp = x0; x0 = x1; x1 = tmp; }
    if (y1 < y0) { int tmp = y0; y0 = y1; y1 = tmp; }

    fastchart_target_gradient_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1,
                                    (uint32_t)chart->gradient_from,
                                    (uint32_t)chart->gradient_to,
                                    (int)chart->gradient_dir);
    return 1;
}

int fastchart_gradient_filled_polygon(fastchart_target_t *t,
                                      fastchart_obj *chart,
                                      const fastchart_point_t *poly, int n_pts)
{
    if (!gradient_active(chart) || n_pts < 3) return 0;

    fastchart_target_gradient_polygon(t, poly, n_pts,
                                       (uint32_t)chart->gradient_from,
                                       (uint32_t)chart->gradient_to,
                                       (int)chart->gradient_dir);
    return 1;
}

void fastchart_filled_polygon_aa(fastchart_target_t *t,
                                 const fastchart_point_t *poly,
                                 int n_pts, int color)
{
    if (n_pts < 3) return;
    /* SVG renderers anti-alias by default; the target's polygon
     * primitive carries the fill as-is and the consumer's rasterizer
     * smooths edges. The dedicated AA-edge pass the libgd path needed
     * (to compensate for gdImageFilledPolygon's hard edges) is
     * unnecessary here. */
    fastchart_target_polygon(t, poly, n_pts, color, 1, 1);
}

void fastchart_filled_wedge_aa(fastchart_target_t *t, int cx, int cy,
                               int diameter, int start_deg, int end_deg,
                               int color)
{
    /* Same AA-by-default reasoning as fastchart_filled_polygon_aa.
     * The arc primitive emits an SVG <path A> arc which renders
     * smooth in any consumer. start_deg / end_deg follow the libgd
     * CCW convention (0 = +x, 3 o'clock). */
    int radius = diameter / 2;
    fastchart_target_arc(t, cx, cy, radius, radius,
                         (double)start_deg, (double)end_deg,
                         color, 1, 1);
}

/* Translate the chart's libgd-convention shadow alpha (0..127,
 * 0 = fully opaque, 127 = fully transparent) into the 0..255
 * (255 = fully opaque) alpha used by fastchart_target_color.
 *
 * The old `255 - a * 2` formula left a one-unit floor: input 127
 * mapped to 1/255, not 0, so a user requesting an invisible drop
 * shadow still got a faint rgba(0,0,0,0.004) trace in the SVG.
 * Proportional scaling rounds 127 cleanly to 0 while preserving
 * the same opaque endpoint at input 0. */
static int shadow_alpha_to_255(const fastchart_obj *chart)
{
    int a = (int)chart->shadow_alpha;
    if (a < 0) a = 0; else if (a > 127) a = 127;
    return 255 - (a * 255 + 63) / 127;
}

static int shadow_color_handle(fastchart_target_t *t,
                               const fastchart_obj *chart)
{
    int rgb = (int)chart->shadow_color;
    int r = (rgb >> 16) & 0xFF;
    int g = (rgb >>  8) & 0xFF;
    int b =  rgb        & 0xFF;
    return fastchart_target_color(t, r, g, b, shadow_alpha_to_255(chart));
}

void fastchart_shadow_filled_rectangle(fastchart_target_t *t,
                                       fastchart_obj *chart,
                                       int x0, int y0, int x1, int y1)
{
    if (!chart->has_drop_shadow) return;
    if (x1 < x0) { int tmp = x0; x0 = x1; x1 = tmp; }
    if (y1 < y0) { int tmp = y0; y0 = y1; y1 = tmp; }
    int handle = shadow_color_handle(t, chart);
    if (handle < 0) return;
    fastchart_target_rect(t,
        x0 + (int)chart->shadow_dx,
        y0 + (int)chart->shadow_dy,
        x1 - x0 + 1, y1 - y0 + 1,
        handle, 1, 0);
}

void fastchart_shadow_filled_polygon(fastchart_target_t *t,
                                     fastchart_obj *chart,
                                     const fastchart_point_t *poly, int n_pts)
{
    if (!chart->has_drop_shadow || n_pts < 3) return;
    int handle = shadow_color_handle(t, chart);
    if (handle < 0) return;
    int dx = (int)chart->shadow_dx, dy = (int)chart->shadow_dy;
    fastchart_point_t stack_pts[256];
    fastchart_point_t *pts = stack_pts;
    if (n_pts > 256) {
        pts = emalloc(sizeof(*pts) * (size_t)n_pts);
    }
    for (int i = 0; i < n_pts; i++) {
        pts[i].x = poly[i].x + dx;
        pts[i].y = poly[i].y + dy;
    }
    fastchart_target_polygon(t, pts, n_pts, handle, 1, 0);
    if (n_pts > 256) efree(pts);
}

void fastchart_shadow_filled_arc(fastchart_target_t *t,
                                 fastchart_obj *chart,
                                 int cx, int cy, int diameter,
                                 int start_deg, int end_deg)
{
    if (!chart->has_drop_shadow) return;
    int handle = shadow_color_handle(t, chart);
    if (handle < 0) return;
    int radius = diameter / 2;
    fastchart_target_arc(t,
        cx + (int)chart->shadow_dx,
        cy + (int)chart->shadow_dy,
        radius, radius,
        (double)start_deg, (double)end_deg,
        handle, 1, 0);
}
