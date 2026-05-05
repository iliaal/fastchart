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

#ifndef FASTCHART_EFFECTS_H
#define FASTCHART_EFFECTS_H

#include "php_fastchart.h"
#include <gd.h>

/* Configure libgd's "styled" line color so subsequent gdImageLine
 * calls using gdStyled paint dashed/dotted segments. The style array
 * is owned by the gdImage internally; safe to call repeatedly. No-op
 * when chart->line_style == LINE_SOLID. Returns the color to actually
 * pass to gdImageLine: gdStyled when a non-solid style is active,
 * otherwise the original color. */
int fastchart_apply_line_style(gdImagePtr im, fastchart_obj *chart, int color);

/* Linearly interpolate between two 24-bit RGB ints. t in [0,1]. */
int fastchart_lerp_rgb(int from, int to, double t);

/* Per-render gradient LUT cache. The 256-entry table is expensive to
 * build (256 gdImageColorAllocate calls) and the (gradient_from,
 * gradient_to) pair is constant across one draw, so renderers
 * allocate one of these on the stack at the top of their draw and
 * pass it to every gradient_filled_* call. The first call builds
 * the table; subsequent calls reuse it. */
typedef struct {
    int  lut[256];
    bool built;
} fastchart_gradient_cache;

static inline void fastchart_gradient_cache_reset(fastchart_gradient_cache *c)
{
    c->built = false;
}

/* Fill a rectangle with a vertical or horizontal gradient between
 * gradient_from and gradient_to (chart settings). No-op when gradient
 * is disabled — returns 0 so callers can fall through to a solid
 * fill of their choice. Returns 1 if the gradient was painted. */
int fastchart_gradient_filled_rectangle(gdImagePtr im, fastchart_obj *chart,
                                        fastchart_gradient_cache *cache,
                                        int x0, int y0, int x1, int y1);

/* Fill an arbitrary polygon with the chart's gradient. Same return
 * semantics as the rectangle variant. */
int fastchart_gradient_filled_polygon(gdImagePtr im, fastchart_obj *chart,
                                      fastchart_gradient_cache *cache,
                                      gdPointPtr poly, int n_pts);

/* Filled polygon with anti-aliased edges. libgd's gdImageFilledPolygon
 * paints a hard-edged fill; this helper walks each segment with the
 * AA renderer afterwards so angled edges read as smooth. Use it for
 * any polygon whose visible edges are diagonal — area-chart fills,
 * radar / polar shapes, pie slices via the radial+arc decomposition
 * at the call site. Axis-aligned rectangles don't benefit and should
 * keep using gdImageFilledRectangle directly. */
void fastchart_filled_polygon_aa(gdImagePtr im, gdPointPtr poly,
                                 int n_pts, int color);

/* Filled wedge (pie slice / gauge zone) with anti-aliased outer arc
 * and radial edges. Wraps gdImageFilledArc(... gdPie) and adds the
 * AA outline. start_deg / end_deg follow libgd's CCW convention with
 * 0 = +x (3 o'clock). */
void fastchart_filled_wedge_aa(gdImagePtr im, int cx, int cy,
                               int diameter, int start_deg, int end_deg,
                               int color);

/* Drop-shadow helpers. Each is a no-op when chart->has_drop_shadow is
 * false. The "before" call paints a darkened/colored offset shape
 * underneath; the actual fill follows on top. */
void fastchart_shadow_filled_rectangle(gdImagePtr im, fastchart_obj *chart,
                                       int x0, int y0, int x1, int y1);
void fastchart_shadow_filled_polygon(gdImagePtr im, fastchart_obj *chart,
                                     gdPointPtr poly, int n_pts);
void fastchart_shadow_filled_arc(gdImagePtr im, fastchart_obj *chart,
                                 int cx, int cy, int diameter,
                                 int start_deg, int end_deg);

#endif /* FASTCHART_EFFECTS_H */
