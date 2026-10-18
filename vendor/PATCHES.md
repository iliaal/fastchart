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

Pinned tag `v1.3.3`. Changed files: `plutovg-font.c`,
`plutovg-ft-raster.c`, `plutovg-surface.c`, `plutovg-stb-image.h`,
`plutovg-path.c`. All other vendored files are upstream-verbatim.

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

<!-- Integrator: append further plutovg entries below this line. -->

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
