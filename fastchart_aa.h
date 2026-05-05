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

#ifndef FASTCHART_AA_H
#define FASTCHART_AA_H

#include <gd.h>

/* Per-render rasterizer context. Allocate once at the top of a draw
 * (only when a chart actually needs coverage-based AA — line / scatter
 * / bar / horizontal bar / surface / contour don't), free at the end.
 * NULL on allocation failure. */
typedef struct fastchart_aa_ctx fastchart_aa_ctx;

fastchart_aa_ctx *fastchart_aa_open(void);
void              fastchart_aa_close(fastchart_aa_ctx *ctx);

/* Coverage-based filled polygon. Renders the polygon into a temporary
 * RGBA buffer with proper fractional-alpha edges, then alpha-composites
 * onto the destination gdImage. Replaces gdImageFilledPolygon for
 * shapes whose visible edges are diagonal (area / radar / polar fills).
 *
 * `color` follows gd's truecolor packing (0xAARRGGBB with alpha 0-127
 * where 0 = opaque). The rasterizer translates that to NanoSVG's
 * native alpha-low ARGB internally. n_pts is the polygon's vertex
 * count (silently no-ops below 3). */
void fastchart_aa_filled_polygon(fastchart_aa_ctx *ctx, gdImagePtr im,
                                 const gdPoint *poly, int n_pts,
                                 int color);

/* Coverage-based filled wedge (pie slice / gauge zone). Internally
 * builds a polygon — center + sampled arc points at ~1 sample per 2
 * px of arc length — and routes through fastchart_aa_filled_polygon.
 * start_deg / end_deg follow libgd's convention (0 = +x, increasing CCW).
 * No-op when end_deg <= start_deg or diameter <= 0. */
void fastchart_aa_filled_wedge(fastchart_aa_ctx *ctx, gdImagePtr im,
                               int cx, int cy, int diameter,
                               int start_deg, int end_deg, int color);

/* Coverage-based stroked line. Renders a thick line with proper AA
 * edges — replaces libgd's two-pass thick + AA spine for gauge
 * needles, scatter trend lines, and any other thick diagonal where
 * the brush-stamp aliasing of gdImageSetThickness shows. The stroke
 * uses round caps so endpoints don't have hard corners. Thickness is
 * in pixels. No-op when ctx is NULL or thickness <= 0. */
void fastchart_aa_stroke_line(fastchart_aa_ctx *ctx, gdImagePtr im,
                              int x1, int y1, int x2, int y2,
                              double thickness, int color);

#endif /* FASTCHART_AA_H */
