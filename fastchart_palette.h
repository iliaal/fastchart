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

#include <gd.h>
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
 * resolve to gd-ints via fastchart_target_color_to_gd(t, handle) when
 * passing to a raw gdImage* call. Idempotent: a second call
 * re-allocates. */
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

/* Per-render gd-color cache for per-point color overrides. Renderers
 * that walk N points with arbitrary RGB overrides (Bar, Scatter,
 * Bubble per-point colors) hit gdImageColorAllocate once per point;
 * on truecolor canvases the per-call cost is a hashtable lookup, on
 * paletted canvases each unique color claims a palette slot. The
 * cache memoizes (rgb -> gd handle) so repeated colors collapse to
 * a single allocation per render. 64 slots is enough for any real
 * chart (overflow falls through to gdImageColorAllocate, preserving
 * correctness at the cost of one allocation).
 *
 * Note: this cache speaks raw gd-ints (not target handles) because
 * its callers (Bar/Scatter/Bubble per-point overrides) still drive
 * gdImage* primitives directly. Phase 3 will route them through the
 * target when SVG dispatch lights up. */
typedef struct {
    int rgb[64];     /* -1 = empty slot */
    int handle[64];
} fastchart_color_cache;

static inline void fastchart_color_cache_init(fastchart_color_cache *c)
{
    for (int i = 0; i < 64; i++) c->rgb[i] = -1;
}

static inline int fastchart_color_cache_get(fastchart_color_cache *c,
                                             gdImagePtr im, int rgb)
{
    if (rgb < 0) return -1;
    unsigned h = ((unsigned)rgb * 2654435761u) >> 26;  /* 64 buckets */
    for (int i = 0; i < 64; i++) {
        int slot = (int)((h + (unsigned)i) & 63u);
        if (c->rgb[slot] == rgb) return c->handle[slot];
        if (c->rgb[slot] == -1) {
            int handle = gdImageColorAllocate(im,
                (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
            c->rgb[slot] = rgb;
            c->handle[slot] = handle;
            return handle;
        }
    }
    /* Cache saturated: still correct, just falls back to a fresh
     * allocation. Rare in practice (charts rarely use 64+ unique
     * override colors). */
    return gdImageColorAllocate(im,
        (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

#endif /* FASTCHART_PALETTE_H */
