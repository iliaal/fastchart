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
  stop/image; it does NOT render <text>. fastchart emits glyph paths
  via FT_Outline_Decompose at SVG-build time when SVG_TEXT_PATHS is
  selected; the rasterizer is text-agnostic.
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

/* Worst-case render-work budget for caller-supplied SVG. The pixel caps
 * bound the canvas, not how often each pixel is repainted: rasterization
 * is immediate-mode, so worst-case work is element_count x canvas pixels
 * (65,536 full-canvas shapes ~ 1.1e12 pixel ops, minutes of CPU that no
 * PHP timer can interrupt). 2^32 ops ~ 256 full-canvas passes at 16 Mpx
 * (single-digit seconds) while ordinary icons/logos stay far below. The
 * count includes non-painting elements (g/defs/gradients), so the bound
 * fails closed: over-counting only over-blocks. */
#define FC_MAX_RENDER_OPS    (1LL << 32)

/* Stroke-dash expansion budget for caller-supplied SVG, shared by every
 * dashed path in one document. plutovg caps each path at 2^20 dash
 * commands, but the element budget above counts a dashed path once no
 * matter how many dashes it expands into, so without a shared cap dash
 * work grows with the number of dashed paths. 2^22 lets four paths
 * each expand to the per-path cap; a document that needs more fails. */
#define FC_MAX_DASH_COMMANDS (1L << 22)

/* Stroke outline budget for caller-supplied SVG, in FreeType stroker
 * output points across every stroke in one document. The element budget
 * above counts a path once however many curves it holds, and the stroker
 * turns each curve into ~180 outline points held in native memory that
 * memory_limit never sees (~45 bytes per point for curves between the
 * stroker borders, the exported outline and the rasterizer cells). The
 * document dash budget alone can produce ~10.5M points (four paths at
 * the per-path dash cap), so 3 * 2^22 keeps that working with ~20%
 * headroom; at the cap a curve-heavy document peaks near 550 MB, and a
 * document that needs more fails. */
#define FC_MAX_STROKE_POINTS (3L << 22)

/* Rasterize the given SVG bytes at the requested target dimensions.
 * On success: pix->rgba is emalloc'd, pix->w/h match the requested
 * dims, return 0.
 * On failure: pix->rgba is NULL, return -1. The caller throws. */
int fastchart_rasterize_svg(const char *svg, size_t svg_len,
                            int target_w, int target_h,
                            fastchart_pixels_t *pix);

/* Single-pass: parse the SVG once, apply the public-SVG content
 * policy, read intrinsic dims, validate against (max_dim, max_pixels),
 * rasterize. Used by the Chart::svgToPng/Jpeg/Webp() static methods
 * where the caller has no a-priori knowledge of the input dimensions
 * and would otherwise parse twice (once for dims, once for render).
 * Internal chart renders go through fastchart_rasterize_svg(), which
 * applies no content policy: they embed <use> and data-URI images.
 *
 * Return codes:
 *    0: success. pix filled; *out_w / *out_h reflect actual dims.
 *   -1: SVG could not be parsed.
 *   -2: SVG has no resolvable intrinsic dimensions.
 *   -3: dims exceed the supplied caps (dims still filled into out_w/h
 *       so the caller can build a meaningful error message).
 *   -4: rasterize failed after a successful parse + dim check.
 *   -5: element_count x output pixels exceeds the render-work budget.
 *   -6: the parsed tree holds an <image> with a data:image/ href.
 *   -7: the parsed tree holds a <use> element.
 *   -8: stroke dashes exceed FC_MAX_DASH_COMMANDS for the document.
 *   -9: stroke outlines exceed FC_MAX_STROKE_POINTS for the document.
 */
int fastchart_rasterize_svg_with_dims(const char *svg, size_t svg_len,
                                       int max_dim, long long max_pixels,
                                       fastchart_pixels_t *pix,
                                       int *out_w, int *out_h);

/* Pre-warm the un-premultiply LUT at MINIT so the lazy first-call init
 * never races under ZTS. */
void fastchart_rasterize_init(void);

#endif /* FASTCHART_RASTERIZE_H */
