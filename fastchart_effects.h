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

/* Fill a rectangle with a vertical or horizontal gradient between
 * gradient_from and gradient_to (chart settings). Allocates intermediate
 * colors on `im`. No-op if gradient is not enabled. Returns 1 if
 * the gradient was painted, 0 if the caller should fall back to a
 * solid fill in `solid_color`. */
int fastchart_gradient_filled_rectangle(gdImagePtr im, fastchart_obj *chart,
                                        int x0, int y0, int x1, int y1,
                                        int solid_color);

/* Fill an arbitrary polygon with the chart's gradient by drawing
 * horizontal interpolated rows clipped to the polygon's scanline
 * extents. Returns 1 if painted, 0 if the caller should fall back. */
int fastchart_gradient_filled_polygon(gdImagePtr im, fastchart_obj *chart,
                                      gdPointPtr poly, int n_pts,
                                      int solid_color);

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
void fastchart_shadow_text(gdImagePtr im, fastchart_obj *chart,
                           const char *font, double size,
                           int x, int y, double angle,
                           const char *text);

#endif /* FASTCHART_EFFECTS_H */
