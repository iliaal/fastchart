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

  SVG → RGBA rasterization. Wraps plutosvg's document parser and
  plutovg's surface rasterizer behind a single entry point that takes
  the SVG source bytes and target dimensions and fills a
  fastchart_pixels_t.

  plutosvg's element table covers rect/circle/ellipse/line/polygon/
  polyline/path/g/defs/use/symbol/svg/linearGradient/radialGradient/
  stop/image — it does NOT render <text>. fastchart emits glyph paths
  via FT_Outline_Decompose at SVG-build time when SVG_TEXT_PATHS is
  selected (Phase 3); the rasterizer is text-agnostic.
*/

#ifndef FASTCHART_RASTERIZE_H
#define FASTCHART_RASTERIZE_H

#include "php.h"
#include "fastchart_encoder.h"

/* Rasterize the given SVG bytes at the requested target dimensions.
 * On success: pix->rgba is emalloc'd, pix->w/h match the requested
 * dims, return 0.
 * On failure: pix->rgba is NULL, return -1. The caller throws. */
int fastchart_rasterize_svg(const char *svg, size_t svg_len,
                            int target_w, int target_h,
                            fastchart_pixels_t *pix);

#endif /* FASTCHART_RASTERIZE_H */
