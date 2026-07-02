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
