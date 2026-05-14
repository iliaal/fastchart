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

/* v1.0 deferral: a true SVG <linearGradient> with stops is v1.1 work.
 * For now we flatten the from/to pair to the midpoint and emit a
 * solid fill via the target abstraction. Visually distinguishable
 * from a flat fill (the midpoint isn't either endpoint) but doesn't
 * reproduce the smooth band the libgd-era pipeline did. */
static int gradient_solid_color_handle(fastchart_target_t *t,
                                       const fastchart_obj *chart)
{
    int mid = fastchart_lerp_rgb((int)chart->gradient_from,
                                  (int)chart->gradient_to, 0.5);
    return fastchart_target_color_rgb(t, mid);
}

int fastchart_gradient_filled_rectangle(fastchart_target_t *t,
                                        fastchart_obj *chart,
                                        int x0, int y0, int x1, int y1)
{
    if (!gradient_active(chart)) return 0;
    if (x1 < x0) { int tmp = x0; x0 = x1; x1 = tmp; }
    if (y1 < y0) { int tmp = y0; y0 = y1; y1 = tmp; }

    int handle = gradient_solid_color_handle(t, chart);
    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1,
                          handle, 1, 1);
    return 1;
}

int fastchart_gradient_filled_polygon(fastchart_target_t *t,
                                      fastchart_obj *chart,
                                      const fastchart_point_t *poly, int n_pts)
{
    if (!gradient_active(chart) || n_pts < 3) return 0;

    int handle = gradient_solid_color_handle(t, chart);
    fastchart_target_polygon(t, poly, n_pts, handle, 1, 1);
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

void fastchart_shadow_filled_rectangle(fastchart_target_t *t,
                                       fastchart_obj *chart,
                                       int x0, int y0, int x1, int y1)
{
    /* v1.0 deferral: real SVG drop shadow needs a <filter> stanza
     * with <feGaussianBlur> + <feOffset> + a chart-wide <defs>
     * registration. Currently a no-op so the calling renderer
     * proceeds to paint its primary shape unshadowed. */
    (void)t; (void)chart; (void)x0; (void)y0; (void)x1; (void)y1;
}

void fastchart_shadow_filled_polygon(fastchart_target_t *t,
                                     fastchart_obj *chart,
                                     const fastchart_point_t *poly, int n_pts)
{
    /* v1.0 deferral: see fastchart_shadow_filled_rectangle. */
    (void)t; (void)chart; (void)poly; (void)n_pts;
}

void fastchart_shadow_filled_arc(fastchart_target_t *t,
                                 fastchart_obj *chart,
                                 int cx, int cy, int diameter,
                                 int start_deg, int end_deg)
{
    /* v1.0 deferral: see fastchart_shadow_filled_rectangle. */
    (void)t; (void)chart; (void)cx; (void)cy;
    (void)diameter; (void)start_deg; (void)end_deg;
}
