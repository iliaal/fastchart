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

#ifndef FASTCHART_PALETTE_H
#define FASTCHART_PALETTE_H

#include "fastchart_target.h"

#define FASTCHART_PALETTE_SERIES_N 8

typedef struct {
    int bg;
    int plot_bg;
    int axis;
    int grid;
    int text;
    int border;
    int up;
    int down;
    int volume;
    int series[FASTCHART_PALETTE_SERIES_N];
} fastchart_palette;

/* Allocate target colors for `theme` (0=light, 1=dark) and fill in the
 * palette struct. Field values are opaque target color handles —
 * pass directly into the fastchart_target_* primitive API. Idempotent:
 * a second call re-allocates. */
void fastchart_palette_init(fastchart_target_t *t, int theme, fastchart_palette *pal);

/* Forward decl to avoid pulling php_fastchart.h here. */
struct _fastchart_obj;

/* Apply per-instance overrides (setBackgroundColor /
 * setPlotBackgroundColor / setSeriesColors) on top of the
 * theme-derived palette. Re-allocates the affected target colors so
 * the palette struct holds valid handles for `t`. */
void fastchart_palette_apply_overrides(fastchart_target_t *t,
                                        const struct _fastchart_obj *chart,
                                        fastchart_palette *pal);

#endif /* FASTCHART_PALETTE_H */
