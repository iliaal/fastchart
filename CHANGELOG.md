# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-05-14

This release rebuilds the rendering pipeline around an SVG-canonical
architecture. SVG is now the source of truth; every raster output
(PNG / JPG / WebP) is produced by flattening text to glyph paths,
rasterizing through plutovg, and encoding with libpng / libjpeg-turbo
/ libwebp. libgd is no longer required at runtime.

### Breaking

- **`draw(\GdImage $canvas)` removed** from every chart class. v1.0
  owns its pixel buffer end-to-end. Callers that previously composited
  multiple charts onto a shared canvas should now stitch via
  `drawSvgFragment()` into one SVG document, or call `renderPng()` and
  composite the decoded bytes in userland.
- **`renderGif()` and `renderAvif()` removed** from `Chart` and
  `Symbol`. Both raise `\Error` if called. Use `renderPng()`,
  `renderJpeg()`, `renderWebp()`, or `renderSvg()` instead. The
  `renderToFile('out.gif' | 'out.avif')` paths reject with the same
  error.
- **`ext/gd` is no longer a runtime requirement.** Loading fastchart
  no longer triggers `\GdImage` class lookup at MINIT. ext/gd can be
  absent and fastchart still functions — but if you still call
  `imagecreatefromstring()` to consume `renderPng()` output, you
  obviously still want it loaded.
- **Default JPEG quality is now 88** (was 90). Matches the
  evaluation-validated sweet spot for the new plutovg + libjpeg-turbo
  encoder. Tunable via `setJpegQuality()` or per-call `renderJpeg(int)`.
- **Pixel output is no longer byte-identical to 0.x.** plutovg's
  rasterizer produces smoother anti-aliasing than libgd; byte-compare
  baselines against 0.x output will fail. Visual quality is improved,
  especially on diagonal lines and glyph edges.

### Added

- **plutovg + plutosvg rasterizer**, vendored under `vendor/plutovg/`
  and `vendor/plutosvg/`. Builds with `PLUTOVG_BUILD_STATIC` +
  `PLUTOSVG_BUILD_STATIC` + `-fvisibility=hidden` so the library
  symbols stay internal to fastchart.so. The rasterizer has no text
  support of its own; fastchart flattens text at SVG-build time.
- **`Chart::setSvgTextMode(int)`** with class constants
  `SVG_TEXT_PATHS` (default; every `<text>` becomes a
  `<g><path d="…"/></g>` via FreeType outline decomposition — self-
  contained SVG, renders in any rasterizer) and `SVG_TEXT_NATIVE`
  (raw `<text>` elements, smaller files, requires consumer text
  support). Symbol gains the same setter.
- **`Chart::setJpegQuality(int)`** (1..100, default 88). Affects
  `renderJpeg()` and `renderToFile('*.jpg')`. Symbol mirrors.
- **Raster encoders** wired directly: PNG via libpng with pHYs DPI
  metadata; JPEG via libjpeg-turbo with `optimize_coding=TRUE`, 4:2:0
  subsampling, and density_unit metadata; WebP via libwebp's
  `WebPEncodeRGBA`. No libgd encoder path remains on the raster
  output path.
- **SVG output.** `Chart::renderSvg()` returns the full document;
  `Chart::drawSvgFragment()` returns a `<g class="fastchart">…</g>`
  group with no outer envelope for stitching multiple charts into one
  caller-managed SVG. `renderToFile()` routes `.svg` through the
  vector path. Same surface on Symbol.
- Internal render-target abstraction (`fastchart_target_t`) with two
  backends: GD-wrapping for the unchanged raster path and SVG-emitting
  via `smart_str`. The axis, text, and palette helpers operate on the
  abstraction; chart families thread the target down to the primitive
  layer. SVG primitives use native `<text>` (no path-embedded glyphs)
  with the font family resolved via FreeType. SVG output is DPI-
  invariant — vector strokes scale infinitely, so `setDpi()` no longer
  inflates the SVG viewport while the raster path retains DPI scaling.

### Fixed
- Layout reservation for 45° rotated X-axis labels accounted for only
  the right-end anchor's projection (`width * 0.707`), not the up-left
  extent of the rest of the rotated text. Long labels like "Jan 2025"
  ran off the bottom of the canvas in raster output too. Reservation
  now uses the full `width * 1.414` projection.
- Layout reservation for the rightmost X-axis tick label measured a
  fixed `"999999"` numeric probe, even when the caller set
  `setCategoryLabels()` with longer strings. The X-label measurement
  path now walks the supplied category labels and uses the widest,
  mirroring the existing Y-axis logic.
- Right margin without a secondary Y axis was bare `MARGIN_RIGHT_PAD`
  next to a full Y-axis label reservation on the left, which read as
  visibly off-center. Right margin now anchors at `left / 2` when both
  Y and X axes are present.
- `StockChart`: first and last candles straddled the Y-axis line and
  right edge because the time domain mapped `t_min` to exactly
  `plot.x0`. The time domain is now padded by half a bar-step on each
  end, matching the half-cell offset categorical X axes already use.
- `BubbleChart`: range computation didn't reserve headroom for the
  bubble radius, so the largest bubble could clip past `plot.y0` /
  `plot.x1` on data sets where the data extremum sat near a niced
  tick boundary. `xmin`/`xmax`/`ymin`/`ymax` now pad by 10% of the
  data span before nice-tick rounding.

### Changed

- **Build dependencies.** Drop `libgd-dev`. Add `libpng-dev`,
  `libjpeg-turbo-dev` (or `libjpeg-dev`), `libwebp-dev`, and
  `libfreetype-dev`. config.m4 probes all four via pkg-config.

### Followups landed (post-Phase 6)

- **libgd link removed.** Dead `gdImage*` branches stripped from
  every chart family body; `fastchart_text.c` swapped libgd's
  `gdImageStringFTEx(NULL, …)` measurement path for direct
  `FT_Set_Char_Size` + `FT_Load_Glyph` advance summation;
  `config.m4` drops the libgd probe entirely. `ldd modules/fastchart.so`
  shows only libfreetype, libpng, libjpeg, libwebp.
- **Effects on the SVG path.** `fastchart_effects.c`
  `gradient_filled_*` now emits `<linearGradient>` defs with stops
  at the chart's `gradient_from`/`_to`; `shadow_filled_*` emits an
  offset-duplicate shape with the shadow color before the main
  draw. (Hard-edged shadow rather than Gaussian-blurred — plutosvg
  doesn't parse `<filter>`.) Wired into Bar and Pie families.
- **Background image / icon SVG emission.** `fastchart_target_image()`
  loads the source file, base64-encodes the bytes, and emits
  `<image href="data:image/png;base64,…" preserveAspectRatio="none">`.
  Source-image byte and dimension caps (8 MiB / 4096 px / 16 Mpx)
  enforced at render time; open_basedir re-checked. PNG and JPEG
  source files only — WebP/GIF/AVIF source files are silently
  skipped because plutosvg's data-URI loader handles only those two.
- **Pixel-tolerance test sweep.** 7 tests that scanned for exact
  RGB matches now use `fc_color_near()` which accepts any AA-blended
  version of the target color against white (alpha 0.3..1.0, ±4 RGB
  per channel). Plutovg's 1px stroke centerline lands at ~50%
  coverage; libgd produced exact-color centerlines.
- **Test EXTENSIONS.** 33 phpt files that round-trip raster output
  via `imagecreatefromstring()` + `imagecolorat()` for pixel
  inspection now declare `gd` in their `--EXTENSIONS--` block, so
  they SKIP cleanly when ext/gd isn't loaded rather than fatal.

### Result

96/96 phpt tests pass with ext/gd loaded; 31 pass + 65 SKIP without
ext/gd. libgd is no longer linked into `modules/fastchart.so`.

## [0.2.0] - 2026-05-09

### Added
- **Symbol family.** Two-class symbology hierarchy parallel to `Chart`:
  `FastChart\Code128` (1D barcode, ISO/IEC 15417) and
  `FastChart\QrCode` (2D matrix code, ISO/IEC 18004). Render-only API
  (`renderPng()`, `renderJpeg()`, `renderWebp()`, `renderGif()`,
  `renderAvif()`, `renderToFile()`) — Symbol classes do not accept a
  caller-supplied `\GdImage`. `Code128` auto-switches between subsets
  A/B/C with an odd-tail-to-C optimisation; mod-103 checksum is
  appended automatically; `setShowText(true)` renders the human-
  readable payload below the bars. `QrCode` ships with all four ECC
  levels (`ECC_L`/`M`/`Q`/`H`) and versions 1..40 via the vendored
  nayuki QR encoder under `vendor/qrcodegen/`. Both classes honour
  `setSize()`, `setQuietZone()`, `setForeground()`, `setBackground()`,
  `setTransparentBackground()`, and `setDpi()` (with PNG/JPEG metadata
  written via `gdImageSetResolution`).

### Fixed
- `StockChart::setOhlcv()` now clears overlay (Bollinger / SAR) and
  indicator-pane buffers when the new candle count is shorter than
  the previous one. Previously the overlay's `n` field kept the old
  length and the renderer walked off the end of the new candle
  array — a use-after-realloc OOB read on the candle pointer.
- Setters that accept paths (`setFont`, `setBackgroundImage`,
  `addIconAt`) now throw an explicit `Error` before
  `RETURN_THROWS` when `php_check_open_basedir` blocks the path.
  `php_check_open_basedir` only emits `E_WARNING` and does not set
  `EG(exception)`, so `RETURN_THROWS` asserted under a debug PHP
  build.
- `QrCode::setQuietZone()` rejects quiet zones above 256 modules
  with a `ValueError` instead of silently clamping.

### Changed
- `composer.json` license expression is now
  `(BSD-3-Clause AND MIT)`. fastchart's own code is unchanged
  BSD-3-Clause; the composite expression declares the MIT-licensed
  nayuki QR encoder vendored under `vendor/qrcodegen/`.

## [0.1.1] - 2026-05-06

### Fixed
- Module load order: declare `ZEND_MOD_REQUIRED("gd")` on the module
  entry so the engine reorders MINIT regardless of `php.ini` / conf.d /
  `-d extension=` order. Previously, configurations that loaded
  `fastchart` before `gd` (notably `docker-php-ext-enable`'s
  alphabetical `conf.d/*.ini`) failed startup with "GdImage class not
  found." `PHP_ADD_EXTENSION_DEP` in `config.m4` is build-system-only
  and does not affect runtime ordering.

### Added
- `scripts/pie-smoke.sh`: PIE install + functional smoke test that
  runs against `php:8.x-cli` with `libgd-dev` and `ext/gd` provisioned
  in the container. Wired into `.release-config` as `smoke_test`.

## [0.1.0] - 2026-05-06

### Added
- Initial public release of fastchart.

[Unreleased]: https://github.com/iliaal/fastchart/compare/0.2.0...HEAD
[0.2.0]: https://github.com/iliaal/fastchart/releases/tag/0.2.0
[0.1.1]: https://github.com/iliaal/fastchart/releases/tag/0.1.1
[0.1.0]: https://github.com/iliaal/fastchart/releases/tag/0.1.0
