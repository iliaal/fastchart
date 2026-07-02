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

/* Drop-shadow helpers. v1.0 no-ops; real SVG <filter feGaussianBlur>
 * emission is v1.1. */
void fastchart_shadow_filled_rectangle(fastchart_target_t *t,
                                       fastchart_obj *chart,
                                       int x0, int y0, int x1, int y1);
void fastchart_shadow_filled_arc(fastchart_target_t *t,
                                 fastchart_obj *chart,
                                 int cx, int cy, int diameter,
                                 int start_deg, int end_deg);

#endif /* FASTCHART_EFFECTS_H */
