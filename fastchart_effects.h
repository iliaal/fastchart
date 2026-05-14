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
#include "fastchart_target.h"

/* Linearly interpolate between two 24-bit RGB ints. t in [0,1]. */
int fastchart_lerp_rgb(int from, int to, double t);

/* Fill a rectangle with the chart's gradient. No-op when gradient is
 * disabled (returns 0). v1.0 emits a solid mid-stop fill via the
 * target abstraction; real <linearGradient> emission is v1.1. */
int fastchart_gradient_filled_rectangle(fastchart_target_t *t,
                                        fastchart_obj *chart,
                                        int x0, int y0, int x1, int y1);

/* Fill an arbitrary polygon with the chart's gradient. Same v1.0
 * deferral as the rectangle variant. */
int fastchart_gradient_filled_polygon(fastchart_target_t *t,
                                      fastchart_obj *chart,
                                      const fastchart_point_t *poly, int n_pts);

/* Filled polygon. SVG renderers anti-alias by default, so the
 * dedicated AA-edge pass the libgd-era helper needed is unnecessary;
 * this stays as a separate entry point for callsite compatibility. */
void fastchart_filled_polygon_aa(fastchart_target_t *t,
                                 const fastchart_point_t *poly,
                                 int n_pts, int color);

/* Filled wedge (pie slice / gauge zone). start_deg / end_deg follow
 * libgd's CCW convention with 0 = +x (3 o'clock). */
void fastchart_filled_wedge_aa(fastchart_target_t *t, int cx, int cy,
                               int diameter, int start_deg, int end_deg,
                               int color);

/* Drop-shadow helpers. v1.0 no-ops; real SVG <filter feGaussianBlur>
 * emission is v1.1. */
void fastchart_shadow_filled_rectangle(fastchart_target_t *t,
                                       fastchart_obj *chart,
                                       int x0, int y0, int x1, int y1);
void fastchart_shadow_filled_polygon(fastchart_target_t *t,
                                     fastchart_obj *chart,
                                     const fastchart_point_t *poly, int n_pts);
void fastchart_shadow_filled_arc(fastchart_target_t *t,
                                 fastchart_obj *chart,
                                 int cx, int cy, int diameter,
                                 int start_deg, int end_deg);

#endif /* FASTCHART_EFFECTS_H */
