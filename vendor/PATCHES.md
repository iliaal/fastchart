# Vendor patch log

Every local modification to the vendored libraries under `vendor/`,
measured against the pinned upstream release. Re-apply each entry (or
confirm it landed upstream) whenever a library is re-vendored, then update
its pin in `.upstream/<library>.yml`.

New local edits carry a `LOCAL-PATCH (fastchart):` comment in the source.
Older entries below use other spellings or no marker at all; the
**Marker** field records which.

| Library | Upstream | Pinned | Pin file |
|---|---|---|---|
| plutovg | https://github.com/sammycage/plutovg | tag `v1.3.3` | `.upstream/plutovg.yml` |
| plutosvg | https://github.com/sammycage/plutosvg | tag `v0.0.8` | `.upstream/plutosvg.yml` |
| qrcodegen | https://github.com/nayuki/QR-Code-generator | `993ccbb6765327f5c49939ba77736098c04ab81d` | `.upstream/qrcodegen.yml` |

Files not listed under a library match the pinned upstream byte for byte.
Verified 2026-10-18 by diffing every vendored file against the pinned
upstream file fetched from GitHub.

## Entry format

- **File:function**: location of the change.
- **What / why**: the change and the defect or cost it addresses.
- **Origin**: fastchart commit that introduced it (later commits that
  reworked it follow in order).
- **Upstream**: whether upstream has it, and what to do on re-vendor.
- **Marker**: the comment that tags the change in the source.

## plutovg

Pinned tag `v1.3.3`. Changed files: `plutovg.h`, `plutovg-blend.c`,
`plutovg-canvas.c`, `plutovg-font.c`, `plutovg-ft-raster.c`,
`plutovg-ft-raster.h`, `plutovg-ft-stroker.c`, `plutovg-ft-stroker.h`,
`plutovg-path.c`,
`plutovg-private.h`, `plutovg-rasterize.c`, `plutovg-stb-image.h`,
`plutovg-surface.c`, `plutovg-utils.h`. All other vendored files are
upstream-verbatim.

### PV-1: portable `PATH_MAX`

- **File:function**: `source/plutovg-font.c`: include block of the POSIX
  font-directory scanner.
- **What / why**: upstream includes `<linux/limits.h>` under `__linux__`,
  a kernel UAPI header that musl/Alpine lack without `linux-headers`.
  Replaced with `<limits.h>` plus a `PATH_MAX 4096` fallback.
- **Origin**: `68c4953`.
- **Upstream**: merged as sammycage/plutovg#81 (`3f4c328`, on `main`).
  Not in tag `v1.3.3`. Drop this patch when re-vendoring any release
  that contains `3f4c328`.
- **Marker**: `fastchart local patch (re-apply on plutovg re-vendor)`.

### PV-2: 32 KB raster cell pool

- **File:function**: `source/plutovg-ft-raster.c`:
  `PVG_FT_MINIMUM_POOL_SIZE`.
- **What / why**: raises the stack cell pool from 8 KB to 32 KB. Large
  chart fills (backgrounds, bars, plot rects) overflowed 8 KB and paid a
  heap malloc plus a re-render pass per primitive.
- **Origin**: `69af40e` (raster optimization #10).
- **Upstream**: sammycage/plutovg#76 (merged, on `main`, not in `v1.3.3`)
  wraps the define in `#ifndef`. On re-vendor past it, drop the edit and
  pass `-DPVG_FT_MINIMUM_POOL_SIZE=32768` from `config.m4` instead.
- **Marker**: `LOCAL-PATCH: opt #10`.

### PV-3: full-width scanline bounds check

- **File:function**: `source/plutovg-ft-raster.c`: `gray_set_cell`.
- **What / why**: upstream casts `ey` and `count_ey` to `unsigned int`,
  so a 64-bit `ey` that is a multiple of 2^32 wraps to a small value,
  passes the check, and indexes `ycells[]` far out of bounds (CWE-681 to
  CWE-787). The check now compares at full width and rejects `ey < 0`.
- **Origin**: `0b555d1`.
- **Upstream**: not submitted. `main` still has the `unsigned int` cast.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PV-4: compile out the image-write side

- **File:function**: `source/plutovg-surface.c`: stb_image_write
  include and the `plutovg_surface_write_to_*` functions.
- **What / why**: guarded by `PLUTOVG_DISABLE_IMAGE_WRITE`, which
  `config.m4` defines. fastchart encodes through libpng, libjpeg-turbo,
  and libwebp and never calls these; the guard removes about 62 KB from
  `fastchart.so`.
- **Origin**: `e758acd`.
- **Upstream**: merged as sammycage/plutovg#75 with the same macro name
  (on `main`, not in `v1.3.3`). Drop the edit when re-vendoring past it.
- **Marker**: `LOCAL-PATCH (fastchart opt #12)`.

### PV-5: PNG and JPEG decoders only

- **File:function**: `source/plutovg-surface.c`: defines
  `STBI_ONLY_PNG` and `STBI_ONLY_JPEG` before the stb_image
  implementation include.
- **What / why**: fastchart only passes PNG and JPEG bytes to plutosvg's
  data-URI loader (`fc_sniff_image_mime` in `fastchart_target.c`). The
  other stb_image decoders are unreachable attack surface and about 70%
  of the header's compiled size.
- **Origin**: `8fe1382`.
- **Upstream**: fastchart-specific reachability choice; not proposed.
  Re-apply on every re-vendor.
- **Marker**: none (explanatory comment only).

### PV-6: validate `plutovg_surface_create_for_data` arguments

- **File:function**: `source/plutovg-surface.c`:
  `plutovg_surface_create_for_data`.
- **What / why**: returns `NULL` for a `NULL` buffer, non-positive
  dimensions or stride, or `width > stride / 4`, and when the surface
  `malloc` fails. Upstream dereferences the failed allocation and trusts
  the geometry.
- **Origin**: `f4724c7`.
- **Upstream**: not submitted. `main` lacks both checks.
- **Marker**: none.

### PV-7: zero JPEG component planes

- **File:function**: `source/plutovg-stb-image.h`:
  `stbi__process_frame_header`.
- **What / why**: `memset`s each component's `raw_data` plane and, for
  progressive JPEGs, its `raw_coeff` plane after allocation. Truncated or
  scan-less JPEGs still decode "successfully" and the resampler or IDCT
  read planes no scan wrote, leaking recycled heap bytes into raster
  output (CWE-908).
- **Origin**: `0b555d1`.
- **Upstream**: not in plutovg's bundled stb_image (`main` is identical
  to `v1.3.3`). Not submitted to plutovg or nothings/stb.
- **Marker**: `LOCAL-PATCH (fastchart)` (two sites).

### PV-8: zero the PNG palette buffer

- **File:function**: `source/plutovg-stb-image.h`:
  `stbi__parse_png_file`, `palette[1024]`.
- **What / why**: stb_image decodes paletted PNGs whose indices exceed
  `pal_len` and reads `palette[idx*4..]` from uninitialized stack, a
  stack info-leak through `renderPng()` / `renderJpeg()` /
  `renderWebp()`. Zero-initialized so out-of-range indices read black.
- **Origin**: `721d031`.
- **Upstream**: nothings/stb#1863 (open as of 2026-10-18) carries the
  same fix. Absent from plutovg's bundled copy (`main` and `v1.3.3`).
  Drop together with PV-9 once a plutovg release bundles it.
- **Marker**: `fastchart-local`.

### PV-9: zero `tc16`

- **File:function**: `source/plutovg-stb-image.h`:
  `stbi__parse_png_file`, `tc16[3]`.
- **What / why**: zero-initializes the 16-bit transparency key, as `tc`
  already is. `stbi__compute_transparency16` reads `tc16[]` entries that
  `stbi__parse_png_file` never set when a 16-bit PNG's `tRNS` skips
  channels, leaking stack bytes into the output raster.
- **Origin**: `4cf976e`.
- **Upstream**: same as PV-8: part of nothings/stb#1863 (open), absent
  from plutovg's bundled copy.
- **Marker**: none.

### PV-10: bounded PNG zlib output

- **File:function**: `source/plutovg-stb-image.h`:
  `stbi__parse_png_file`, IDAT decode.
- **What / why**: decodes into a fixed buffer of `2 * raw_len + 64` bytes
  sized from the IHDR dimensions instead of stb's doubling buffer, which
  grows to about 2 GiB for a PNG whose IDAT inflates past its declared
  size (zip bomb, CWE-409), invisible to `memory_limit`. Over-long
  streams now fail the decode.
- **Origin**: `0b555d1`.
- **Upstream**: not submitted.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PV-11: cubic flattening work caps

- **File:function**: `source/plutovg-path.c`:
  `plutovg_path_traverse_flatten`.
- **What / why**: caps subdivision at 4096 segments per cubic and
  2^21 segments per path. Cubics with control points near 1e15 never
  meet the 0.25 flatness tolerance and would split every node to depth
  31; a large `d` attribute multiplies that across many cubics
  (CWE-400).
- **Origin**: `0b555d1`.
- **Upstream**: not submitted.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PV-12: per-path dash budget and float-freeze guard

- **File:function**: `source/plutovg-path.c`: `dasher_t`,
  `dash_traverse_func`, `plutovg_path_traverse_dashed`.
- **What / why**: every emitted dash command, including subpath-start
  `MOVE_TO`s, decrements a per-path budget of 2^20 (`1 << 20`). A segment
  whose length is non-finite or at least 2^23 times the smallest dash
  increment, where the float32 cursor stops advancing, is emitted as one
  span instead of looping forever (CWE-400).
- **Origin**: `0b555d1`.
- **Upstream**: not submitted. Upstream has no dash cap.
- **Marker**: `LOCAL-PATCH (fastchart)` (three sites).

### PV-13: path-data element cap

- **File:function**: `source/plutovg-path.c`: `plutovg_path_parse`.
- **What / why**: stops parsing once a path exceeds 2^20 elements and
  returns `false`. One giant `d` attribute otherwise amplifies a 16 MB
  input into hundreds of MB of native memory outside `memory_limit`
  (CWE-400/CWE-770). plutosvg renders the parsed prefix (PS-4).
- **Origin**: `0b555d1`.
- **Upstream**: not submitted.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PV-14: decoder image ceiling

- **File:function**: `include/plutovg.h`: `PLUTOVG_IMAGE_MAX_DIMENSION`;
  `source/plutovg-surface.c`: stb configuration.
- **What / why**: sets `STBI_MAX_DIMENSIONS` from
  `PLUTOVG_IMAGE_MAX_DIMENSION` (4096) instead of stb's default 2^24, so
  stb refuses any image larger than 4096 px on a side on every decode
  path. Before, a small compressed PNG or JPEG could drive a decode of
  about 1 GiB outside `memory_limit` whenever a caller-side size check
  disagreed with stb's own header parse (fch-qz0, CgBI PNGs).
  `fastchart_target.c` fails the build with `#error` if the value
  differs from `FC_IMAGE_MAX_DIM`.
- **Origin**: `0ebdddf`, `69cb5b5`.
- **Upstream**: not proposed; an embedder policy that stb leaves
  configurable.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PV-15: `plutovg_image_info_from_data`

- **File:function**: `include/plutovg.h`, `source/plutovg-surface.c`:
  `plutovg_image_info_from_data`.
- **What / why**: wraps `stbi_info_from_memory` so fastchart sizes and
  caps source images from the decoder's own header parse rather than a
  separate sniff that a leading `CgBI` chunk could fool (fch-qz0).
- **Origin**: `0ebdddf`.
- **Upstream**: not submitted; a candidate info-only counterpart to
  `plutovg_surface_load_from_image_data`.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PV-16: non-finite arc sweeps

- **File:function**: `source/plutovg-path.c`: `plutovg_path_arc_to`,
  `plutovg_path_add_arc`.
- **What / why**: a non-finite sweep made `(int)ceilf(...)` of the
  segment count undefined; optimized builds emitted cubics until memory
  ran out (3.1 GB in under 2 s). `arc_to` now draws a line to the end
  point, as SVG does for degenerate arcs, and `add_arc` returns early,
  which also removes the `2 + 4 * seg_n` reserve overflow (fch-tvt).
- **Origin**: `350bf63`.
- **Upstream**: unfixed on `main` (`f63f9b5`); PR pending.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PV-17: checked array and cache growth

- **File:function**: `source/plutovg-utils.h`: `plutovg_array_ensure`
  (now `plutovg_array_reserve`, returns `bool`); `source/plutovg-path.c`
  callers; `source/plutovg-canvas.c` `append_data`;
  `source/plutovg-font.c`: `plutovg_glyph_cache_get`,
  `plutovg_font_face_cache_add`, `plutovg_font_face_cache_load_file`.
- **What / why**: growth assigned the `realloc` result unchecked and
  doubled an `int` capacity, so allocation failure or overflow wrote
  through NULL. Growth now detects both, frees and empties the array,
  and callers drop the command; font-cache allocations return NULL or
  skip the entry. In `source/plutovg-rasterize.c`, span-buffer growth
  resets the buffer on failure, `ft_outline_create` rejects overflowing
  counts and checks `malloc`, and NULL outlines (including a failed
  stroker) propagate to `plutovg_rasterize`, which draws nothing
  (fch-tvt).
- **Origin**: `350bf63`, `a0e9173`, `82b30ed`.
- **Upstream**: unfixed on `main`; PR pending.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PV-18: per-canvas dash budget

- **File:function**: `include/plutovg.h`: `plutovg_canvas_set_dash_budget`,
  `plutovg_canvas_dash_budget_exceeded`; `source/plutovg-canvas.c`;
  `source/plutovg-private.h`: `plutovg_dash_budget_t`;
  `source/plutovg-path.c`: `plutovg_path_traverse_dashed`.
- **What / why**: PV-12's 2^20 cap applied per path, so a document of
  many dashed paths still cost tens of seconds. A canvas can now carry
  one budget that all its strokes share; fastchart's `svgTo*()` sets
  2^18 dash commands per document, sized so it runs out before the
  PV-22 stroke budget, and throws `ValueError` when it does. Charts pass
  -1 and keep only PV-12 (fch-6nw).
- **Origin**: `fa334d5`, `1e9123d`, `39d1b7f`.
- **Upstream**: not proposed; upstream has no dash cap at all.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PV-19: clip geometry before fixed-point conversion

- **File:function**: `source/plutovg-rasterize.c`:
  `ft_outline_convert_path`, `ft_path_clip`, `ft_outline_convert_stroke`,
  `plutovg_span_buffer_contains`.
- **What / why**: `FT_COORD` cast floats to 26.6 fixed point unchecked,
  and the FreeType-derived stroker and rasterizer overflow well before
  that cast does. Paths with a mapped point beyond +/-2^21 px are clipped
  to that box first (lines split on the box edge, winding preserved
  inside it; cubics halved, then chorded). Infinite coordinates map to
  +/-`FLT_MAX` and NaN paths are dropped. Stroke half-width is capped at
  2^19 px and radius x miter at 2^20 px (miter <= 2^14), so every
  product stays below 2^62.2. Paths entirely inside the box take the
  original path, so ordinary output is byte-identical. Visible change: a
  stroke wider than about 2^20 px draws at the cap, so a round-capped
  segment 10^6 px off-canvas with width 2.1*10^6 no longer covers the
  canvas. `plutovg_span_buffer_contains` rejects input outside `int`
  range (fch-lw5, fch-son).
- **Origin**: `ac8e406`, `fb5fece`, `fea52d0`.
- **Upstream**: unfixed on `main` (`f63f9b5`); PR pending.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PV-20: empty stroke border export

- **File:function**: `source/plutovg-ft-stroker.c`:
  `ft_stroke_border_export`.
- **What / why**: returns early on an empty border instead of calling
  `memcpy` with a NULL source (UBSan) (fch-son).
- **Origin**: `ac8e406`.
- **Upstream**: unfixed on `main`; PR pending.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PV-21: gradient position saturation

- **File:function**: `source/plutovg-blend.c`: `gradient_pixel`,
  `fetch_linear_gradient`.
- **What / why**: degenerate gradient geometry (a far-off radial focus, a
  near-zero linear vector) converted NaN or out-of-range floats to `int`.
  Positions now saturate at +/-2^30 with NaN mapped to 0, and `t` is
  range-checked before the fixed-point path, so pad spread paints the
  last stop as SVG requires (fch-son). The texture fetchers'
  float-to-int casts (`fetch_*_transformed`, about lines 737-956) are
  not changed; fastchart feeds them integer image geometry only.
- **Origin**: `2e5f16b`.
- **Upstream**: unfixed on `main`; PR pending.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PV-22: per-canvas stroke outline budget

- **File:function**: `include/plutovg.h`:
  `plutovg_canvas_set_stroke_budget`,
  `plutovg_canvas_stroke_budget_exceeded`; `source/plutovg-canvas.c`;
  `source/plutovg-private.h`; `source/plutovg-path.c`;
  `source/plutovg-ft-stroker.[ch]`: `PVG_FT_Stroker_SetMaxPoints`,
  `PVG_FT_Stroker_ParseOutline`; `source/plutovg-rasterize.c`:
  `ft_outline_convert_stroke`.
- **What / why**: stroker output for in-range geometry had no bound; one
  path of 300,000 stroked cubics took 23.7 s and 2.0 GB of native
  memory. A canvas can carry a stroke-point budget shared by all its
  strokes, the stroker stops parsing once a stroke passes it, and an
  over-budget stroke is not drawn. fastchart's `svgTo*()` sets 2^22
  points per document (about 190 MB at worst) and pairs it with a 2^18
  dash-command budget (PV-18) that always runs out first. Charts pass -1
  (fch-lw5). The stroker limit is clamped in `long long`: `(long)UINT_MAX`
  is -1 where `long` is 32 bits, which lifted the limit.
- **Origin**: `0574f8b`, `39d1b7f`; clamp in `f323d11`.
- **Upstream**: not proposed; an embedder budget.
- **Marker**: `LOCAL-PATCH (fastchart)`.

<!-- Integrator: append further plutovg entries below this line. -->

### PV-23: banded rasterizer work bounds

- **File:function**: `source/plutovg-ft-raster.c`: `gray_render_line`,
  `gray_find_cell`, `gray_sort_cells`, `gray_sort_all_cells`,
  `gray_sweep`, `gray_convert_glyph`, `PVG_FT_Raster_Render`.
- **What / why**: three superlinear costs in the gray rasterizer, each
  reachable from a small SVG; output stays byte-identical.
  - Band clip: every raster band re-decomposes the whole outline, and
    `gray_render_line` walked every scanline of an edge for each band,
    so cost was bands x edges x height (a 2 KB path of 250 full-height
    segments took 7.6 s release, 20 s ASan). The walk now jumps to the
    band's first row by advancing the line stepper in closed form and
    stops once the edge leaves the band.
  - Cell walk cap: each coverage record walked the row's sorted cell
    list, so a row crossed by k edges cost k^2. The walk now stops after
    32 cells and pushes the cell unsorted; `gray_sweep` sorts and folds
    such bands, and a full single-row band folds its duplicates before
    it overflows.
  - Pool sizing: with a fixed 32 KB pool the band count grew with the
    cell count, so outlines of many short edges cost edges x cells. The
    first pool is now 64 bytes per outline point within the existing
    8 MB cap. Sizing it for every cell was slower: cell lists spread
    over megabytes miss the cache on each walk step.
- **Origin**: `45a46a9` (fch-au7); markers in `00b8b6f`.
- **Upstream**: PR candidate. plutovg `main` and FreeType's `ftgrays.c`
  share the per-band scanline walk and the sorted cell list.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PV-24: per-canvas raster work budget

- **File:function**: `include/plutovg.h`:
  `plutovg_canvas_set_raster_budget`,
  `plutovg_canvas_raster_budget_exceeded`; `source/plutovg-canvas.c`;
  `source/plutovg-path.c`; `source/plutovg-private.h`;
  `source/plutovg-ft-raster.h`: `PVG_FT_Raster_Budget`,
  `PVG_FT_Raster_Params.budget`; `source/plutovg-ft-raster.c`: cell,
  walk, sort, curve-split and per-band charges, `ErrRaster_Budget`,
  `gray_convert_glyph_inner`, `PVG_FT_Raster_Render`;
  `source/plutovg-rasterize.c`: `plutovg_rasterize`.
- **What / why**: coverage work grows with the rows every edge spans,
  which the element, dash and stroke budgets cannot see; with PV-23 it
  is linear but unbounded (32,000 full-height segments on a 4096 px
  canvas took 20 s release). A canvas can carry a work budget shared by
  every fill, stroke and clip; the rasterizer charges cell steps,
  cell-list walk steps, sort list steps, curve subdivisions, one unit
  per line per band, and one pass over the outline per band. The path that runs it out draws
  nothing, later paths are skipped, and the caller discards the surface.
  fastchart's `svgTo*()` sets 1.2G units per document and refuses with
  "fill coverage too large"; charts pass -1 (fch-au7). The counters and
  the API take `long long`, so an unbudgeted render past 2^31 units does
  not stop part-way where `long` is 32 bits.
- **Origin**: `d8c8629`, `dd3d77a`, `00b8b6f`, `fe0e23a`; 64-bit counters
  in `f323d11`.
- **Upstream**: not proposed; an embedder budget.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PV-25: per-canvas paint budget

- **File:function**: `include/plutovg.h`:
  `plutovg_canvas_set_paint_budget`,
  `plutovg_canvas_paint_budget_exceeded`; `source/plutovg-private.h`
  (canvas field); `source/plutovg-canvas.c`: `plutovg_canvas_create`,
  `plutovg_canvas_paint`, `plutovg_canvas_fill_preserve`,
  `plutovg_canvas_stroke_preserve`; `source/plutovg-blend.c`:
  `plutovg_paint_weight`, `plutovg_paint_budget_charge`, `plutovg_blend`.
- **What / why**: the element x pixel budget counts elements, not paint
  passes or paint kind, so 251 full-canvas radial-gradient rects with
  fill, stroke and opacity at 4096 px took 31.7 s of blending on an idle
  aarch64 host. `plutovg_blend`, which every fill, stroke, paint and
  image draw reaches, now charges each pass the pixels it blends times a
  paint-kind weight (opaque solid 1, translucent solid 7, linear
  gradient 14, radial gradient 20, texture 20), measured at 0.36, 2.5,
  2.6-4.8 and 7.0 ns per pixel. Pixels of a span below full coverage
  take the per-pixel blend path and are charged a partial weight (solid
  8, linear 21, radial and texture 26; measured 2.7, 7.3 and 8.9-9.2 ns),
  so opaque half-coverage strips are no longer charged as cheap opaque
  fills. The pass that would exceed the budget is not drawn and later
  fills and strokes return early. fastchart's `svgTo*()` sets 2^33 units
  per document and refuses with "paint coverage too large"; charts and
  symbols pass -1 (fch-ak7). With the raster, stroke, dash and paint
  budgets all near their caps, the costliest in-budget 4096 px document
  measured renders in 10.9 s on an idle aarch64 host, PNG encoding
  included.
- **Origin**: `f5d2df0`; partial-coverage weights `3f327c3`.
- **Upstream**: not proposed; an embedder budget.
- **Marker**: `LOCAL-PATCH (fastchart)`.

## plutosvg

Pinned tag `v0.0.8`. Changed files: `plutosvg.c`, `plutosvg.h`.
`plutosvg-ft.h` is upstream-verbatim. Upstream `master`
(`fd8a080`) is identical to `v0.0.8` for both changed files.

### PS-1: document complexity caps

- **File:function**: `source/plutosvg.c`:
  `plutosvg_document_load_from_data`, `parse_attributes`,
  `add_attribute`, `parse_style`.
- **What / why**: rejects documents with more than 65536 elements
  (`MAX_DOCUMENT_ELEMENTS`), 262144 attributes including `id` and inline
  `style` declarations (`MAX_DOCUMENT_ATTRIBUTES`), or nesting deeper than
  256 (`MAX_DOCUMENT_DEPTH`). `add_attribute` and `parse_style` now return
  `bool` to propagate the attribute cap.
- **Origin**: `5beb5b0`.
- **Upstream**: not submitted. Upstream caps render recursion
  (`MAX_RENDER_DEPTH`, our merged sammycage/plutosvg#49) but not parse
  size.
- **Marker**: none.

### PS-2: decoded-image cache

- **File:function**: `source/plutosvg.c`: `element_t` image fields,
  `image_cache_*` helpers, `load_image`, `draw_image`,
  `element_destroy_images`, `plutosvg_document_destroy`.
- **What / why**: retains decoded `<image>` surfaces per document so
  repeated `<use>` references decode once. The cache is bounded (default
  64 MiB, `PLUTOSVG_IMAGE_CACHE_LIMIT`) and evicts oldest-first, letting
  an entry go only when its remaining-uses x bytes value is lower than the
  incoming image's. An image that cannot be cached still renders,
  uncached.
- **Origin**: `f4724c7` (per-element cache), `d508bbe` (bounded LRU with
  use-frequency eviction), `6c61dd6` (configurable limit, PS-3).
- **Upstream**: not submitted. Upstream decodes on every draw.
- **Marker**: `LOCAL-PATCH (fastchart repeated images)` on the struct
  fields only.

### PS-3: `plutosvg_document_set_image_cache_limit`

- **File:function**: `source/plutosvg.h`, `source/plutosvg.c`:
  `plutosvg_document_set_image_cache_limit`; `#include <stddef.h>` in the
  header.
- **What / why**: new public function so fastchart can set the PS-2
  ceiling from the `fastchart.max_image_cache_bytes` INI instead of the 64 MiB
  default. `<stddef.h>` provides `size_t` for the new prototypes.
- **Origin**: `6c61dd6`.
- **Upstream**: not submitted.
- **Marker**: `LOCAL-PATCH (fastchart)` in the header doc comment.

### PS-4: `plutosvg_document_element_count`

- **File:function**: `source/plutosvg.h`, `source/plutosvg.c`:
  `plutosvg_document_element_count`; comment in `render_path`.
- **What / why**: exposes the parsed element count so fastchart can
  budget render work (elements x pixel coverage) before rasterizing. The
  `render_path` comment records that it ignores PV-13's `false` return on
  purpose and renders the parsed prefix, as upstream and SVG 2 do.
- **Origin**: `0b555d1`.
- **Upstream**: not submitted.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PS-5: bounded `<use>` expansion

- **File:function**: `source/plutosvg.c`: `render_use`, `render_element`,
  `render_context_t`, `plutosvg_document_render`,
  `plutosvg_document_extents`; `source/plutosvg.h` return-value docs.
- **What / why**: `render_use` called `render_element` directly and
  bypassed `MAX_RENDER_DEPTH`, so a deep `<use>` chain overflowed the
  stack, 2-way fan-out cost CPU exponential in depth, and a `<use>` that
  referenced its own ancestor never terminated. `<use>` hops now count
  toward the depth cap, ancestor references are refused, and expansion
  under `<use>` is capped at 2^20 element visits and 8 MiB of rendered
  attribute bytes (href excluded); render and extents return `false`
  when the budget is hit (fch-q6i).
- **Origin**: `18e5e4c`, `71988ee`.
- **Upstream**: unfixed on `master` (`fd8a080`); PR pending.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PS-6: one attribute record per id

- **File:function**: `source/plutosvg.c`: `add_attribute`.
- **What / why**: duplicate attributes were all kept (up to 262,144), so
  every lookup and PS-5's byte charge walked them. A repeated record now
  overwrites the earlier value in place, which keeps the last-wins
  result and caps each element at one record per attribute id (fch-q6i
  review).
- **Origin**: `5a6b614`.
- **Upstream**: unfixed on `master`; PR pending.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PS-7: `plutosvg_document_content_flags`

- **File:function**: `source/plutosvg.h`, `source/plutosvg.c`:
  `plutosvg_document_content_flags`, `href_has_data_image`.
- **What / why**: reports `<use>` elements and data-URI `<image>`
  elements from the parsed tree, so `svgTo*()` applies its content
  policy to exactly what plutosvg renders. The earlier byte-level
  pre-scan could be bypassed with comment, CDATA or PI openers inside
  attribute values, a DOCTYPE or the XML declaration (fch-ntf).
- **Origin**: `b6f859b`, `5f06114`.
- **Upstream**: fastchart-specific; not proposed.
- **Marker**: `LOCAL-PATCH (fastchart)`.

### PS-8: gradient stops linked at parse time

- **File:function**: `source/plutosvg.c`: `struct element`
  (`first_stop`, `last_stop`, `next_stop`), `plutosvg_document_load_from_data`,
  `resolve_gradient_stops`, `collect_gradient_attributes`.
- **What / why**: every fill or stroke painted with a gradient resolved
  its stops by walking all children of the gradient and each element on
  its `href` chain, `<stop>` or not. A gradient padded with 16,000 `<g/>`
  children and used by 16,000 rects for fill and stroke cost 0.5G child
  visits, 7.4 s against 0.56 s unpadded on a release build (fch-9hm). The
  parser now links each element's `<stop>` children in document order as
  it appends them, and both functions follow that chain; the per-paint
  cost is the stop count. After parsing, only `render_use` touches the
  tree, and it redirects a parent pointer without changing any child
  list, so the chain cannot go stale; `svgTo*()` refuses `<use>` anyway.
  The stops, their order and the 64-stop limit are unchanged: rendered
  bytes match the previous build on every gradient case and corpus
  document checked.
- **Origin**: `224003e`.
- **Upstream**: candidate for an upstream PR; `v0.0.8` and `master`
  walk every child per paint.
- **Marker**: `LOCAL-PATCH (fastchart)`.

<!-- Integrator: append further plutosvg entries below this line. -->

## qrcodegen

Pinned `993ccbb`. `c/qrcodegen.c` and `c/qrcodegen.h` at that commit are
byte-identical to the previous pin `2c9044de6b049ca25cb3cd1649ed7e27aa055138`.
Changed file: `qrcodegen.c`. `qrcodegen.h` is upstream-verbatim.
`LICENSE` and `README.md` are fastchart-authored and have no upstream
counterpart.

### QR-1: GF(2^8) log/antilog tables

- **File:function**: `qrcodegen.c`: `gf_log`, `gf_antilog`,
  `reedSolomonMultiply`.
- **What / why**: replaces upstream's per-call Russian-peasant multiply
  with 256- and 512-byte log/antilog lookups; the multiply runs about 17k
  times for a version 25-H symbol (39k at 40-H). The tables were first built lazily
  behind an unsynchronized `gf_tables_ready` flag, which ZTS threads could
  race on (fch-dqj). They are now `static const` precomputed data, with no
  init path. All 65536 operand pairs match the upstream multiply.
- **Origin**: `8ed1ba0` (lazy tables), `c9bad0b` (const tables).
- **Upstream**: not proposed; upstream keeps the table-free multiply and
  notes a 256x256 table as an alternative.
- **Marker**: `LOCAL-PATCH (fastchart)`.

<!-- Integrator: append further qrcodegen entries below this line. -->
