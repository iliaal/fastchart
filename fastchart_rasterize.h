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
 * PHP timer can interrupt). 2^32 ops ~ 256 full-canvas elements at 16 Mpx
 * while ordinary icons/logos stay far below. The budget counts elements,
 * not paint passes or paint kind: an element can paint twice (fill and
 * stroke), and gradient paint costs about 10x a solid fill, so on its own
 * it admits ~32 s of blending at 4096 px (251 full-canvas radial-gradient
 * rects with fill, stroke and opacity); FC_MAX_PAINT_WORK below bounds
 * that. The count includes non-painting elements (g/defs/gradients), so
 * the bound fails closed: over-counting only over-blocks. */
#define FC_MAX_RENDER_OPS    (1LL << 32)

/* Native-memory budgets for caller-supplied SVG. The element budget above
 * counts a path once however much geometry it expands into, so two
 * expansions get their own per-document caps.
 *
 * FC_MAX_STROKE_POINTS is the ceiling: FreeType stroker output points
 * across every stroke in one document. Those points, the exported
 * outline and the rasterizer cells live in native memory that
 * memory_limit never sees; a stroked curve turns into ~180 points at
 * ~45 bytes each, so the cap bounds a curve-heavy document near 190 MB.
 * Ordinary documents stay far below it: eight stitched charts of eight
 * 2048-point series stroke into ~0.8M points.
 *
 * FC_MAX_DASH_COMMANDS caps dash commands across every dashed path. A
 * dash command strokes into at most ~9 points (measured with round caps
 * and round joins on sharp reversals; butt caps give ~2.5), so 2^18
 * commands use a little over half the stroke budget whatever the cap and
 * join style: a document that fits the dash budget runs out of dash budget,
 * not stroke budget, and keeps room for its other strokes. plutovg still
 * caps a single path at 2^20 commands, which only charts (no shared
 * budget) can reach. */
#define FC_MAX_STROKE_POINTS (1L << 22)
#define FC_MAX_DASH_COMMANDS (1L << 18)

/* Rasterizer work budget for caller-supplied SVG, in plutovg raster work
 * units (see plutovg_canvas_set_raster_budget()): one per cell step and
 * cell-list walk step, per list step of the per-row sort, per curve
 * subdivision, per edge per raster band and per outline point per band,
 * across every fill, stroke and clip in the document. Coverage cost
 * grows with the rows each edge spans, which the element and stroke
 * budgets cannot see: unbudgeted, a 74 KB path of 8,000 full-height
 * segments on a 4096 px canvas is ~3.3G units.
 *
 * The cap is sized to the heaviest ordinary document it must admit:
 * eight stitched 1024x512 line charts of eight 2048-point series in a
 * 2048 px square (~1.01G units, 2.8 s release on an idle aarch64 host).
 * The same document rendered at 2304 px (~1.14G) still renders; at
 * 2560 px (~1.35G) it is refused. On that host the costliest in-budget
 * documents found are 4096 px paths of full-height cubics (4.5 s) and of
 * interleaved zigzags (n=11,750: 7.0 s, about a third of it blending and
 * PNG encoding, which FC_MAX_PAINT_WORK and the pixel caps bound), and
 * a refusal, which spends the whole budget, takes 2.5-3.9 s. An x86 host
 * at load 8-12 measured 1.7-2.4x those figures. */
#define FC_MAX_RASTER_WORK   1200000000LL

/* Blending budget for caller-supplied SVG, in plutovg paint units (see
 * plutovg_canvas_set_paint_budget()): every pixel each fill, stroke or
 * paint pass blends, times a weight for its paint kind (opaque solid 1,
 * translucent solid 7, linear gradient 14, radial gradient 20), so a unit
 * costs about 0.36 ns on an idle aarch64 host. The element x pixel budget
 * above counts elements, not passes or paint kind, so on its own it
 * admits 251 full-canvas radial-gradient rects with fill and stroke at
 * 4096 px (~32 s on an idle aarch64 host). 2^33 units caps blending
 * near 3.1 s there: sixteen full-canvas radial-gradient layers under a
 * line chart at 4096 px (~5.4G) render in 3.6 s, the costliest in-budget
 * document measured (25 such layers, ~8.4G) in 5.2 s with its PNG
 * encoding, and a refusal takes 2.9-3.1 s. */
#define FC_MAX_PAINT_WORK    (1LL << 33)

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
 *  -10: rasterizer work exceeds FC_MAX_RASTER_WORK for the document.
 *  -11: blending work exceeds FC_MAX_PAINT_WORK for the document.
 */
int fastchart_rasterize_svg_with_dims(const char *svg, size_t svg_len,
                                       int max_dim, long long max_pixels,
                                       fastchart_pixels_t *pix,
                                       int *out_w, int *out_h);

/* Pre-warm the un-premultiply LUT at MINIT so the lazy first-call init
 * never races under ZTS. */
void fastchart_rasterize_init(void);

#endif /* FASTCHART_RASTERIZE_H */
