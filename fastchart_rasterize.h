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

/* Output dimension caps. Apply to caller-supplied SVG via the
 * Chart::svgToPng/Jpeg/Webp() entry points and to source images
 * loaded via setBackgroundImage(). Per-axis cap (4096) keeps single
 * dimensions sane for screen output; total-pixel cap (16M) bounds
 * the RGBA buffer below ~64 MB. */
#define FC_IMAGE_MAX_DIM     4096
#define FC_IMAGE_MAX_PIXELS  (16 * 1024 * 1024)

/* Max SVG bytes accepted by Chart::svgToPng/Jpeg/Webp(). Keeps
 * plutosvg's parser from chewing on adversarially huge input. */
#define FC_SVG_MAX_BYTES     (16 * 1024 * 1024)

/* Rasterize the given SVG bytes at the requested target dimensions.
 * On success: pix->rgba is emalloc'd, pix->w/h match the requested
 * dims, return 0.
 * On failure: pix->rgba is NULL, return -1. The caller throws. */
int fastchart_rasterize_svg(const char *svg, size_t svg_len,
                            int target_w, int target_h,
                            fastchart_pixels_t *pix);

/* Parse the SVG just enough to discover its intrinsic width/height
 * (from the root <svg> element's width / height / viewBox). Used by
 * Chart::svgToPng/Jpeg/Webp() to size the output before allocating
 * the rasterize buffer. Returns 0 on success, -1 if the document
 * can't be parsed or has no resolvable intrinsic dimensions. */
int fastchart_svg_get_intrinsic_dims(const char *svg, size_t svg_len,
                                      int *out_w, int *out_h);

#endif /* FASTCHART_RASTERIZE_H */
