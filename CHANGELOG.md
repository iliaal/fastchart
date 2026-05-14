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

### Followups landed (security, leak-clean, optional codecs)

- **Security: open_basedir TOCTOU eliminated in source-image loading.**
  `setBackgroundImage` / `addIconAt` previously sniffed dimensions via
  direct `fopen()`, then stat()-ed, then opened the path through the
  PHP stream layer with `STREAM_DISABLE_OPEN_BASEDIR`. A basedir-
  inside symlink whose target was swapped to outside basedir would
  resolve at the bypass-flagged final open. Collapsed to one
  `php_stream_open_wrapper` without `STREAM_DISABLE_OPEN_BASEDIR`; the
  stream wrapper enforces `open_basedir` natively, no separate check
  to race with. Single open also reads up to `FC_IMAGE_MAX_BYTES + 1`
  so a file at the cap passes while a file over the cap is rejected.
- **FT_Library is now process-shared.** Glyph-path emitter, font-
  family resolver, and text-bbox measurer share one lazily-initialized
  handle; `fastchart_ft_library_shutdown()` releases it at MSHUTDOWN.
  Drops a 2264-byte-per-render LSan-reported leak and removes the
  per-call `FT_Init_FreeType` / `FT_Done_FreeType` overhead on the
  raster path.
- **Optional codec libs.** `libpng` / `libjpeg-turbo` / `libwebp` are
  now probed independently by `config.m4` — each missing lib turns
  the matching `renderXxx()` into a clear "format not compiled in"
  Error at call time. FreeType remains mandatory (text rendering
  depends on it). SVG output stays available regardless of which
  raster codecs are linked in.
- **phpinfo() lib version table.** Per-library version rows for
  FreeType, libpng, libjpeg-turbo, libwebp, plutovg, plutosvg.
  Missing optional libs show `(not compiled in)` so packagers can
  audit a build.
- **AreaChart honours `setGradientFill()`.** Both stacked and non-
  stacked fill paths now route through
  `fastchart_target_gradient_polygon` when a gradient is configured.
  Public docs previously claimed support but `AreaChart`'s polygon
  emit only ever used the solid series color.
- **SVG_TEXT_NATIVE sanitizes XML-invalid bytes.** `fc_svg_escape`
  was escaping only the five XML metacharacters; user-supplied
  titles or labels containing C0 control bytes survived into the
  `<text>` content and made `xmllint` / any conforming parser reject
  the document. Bytes outside TAB / LF / CR / 0x20+ now emit U+FFFD.
- **CI runs with explicit LSan leak detection.** `ASAN_OPTIONS`
  pins `detect_leaks=1`; `.github/lsan-suppressions.txt` covers
  ext/gd's MINIT-time persistent allocations (intentional, not bugs)
  and nothing else. A "Render-only leak smoke" CI step exercises
  fastchart without ext/gd loaded — any `LeakSanitizer` output is a
  fastchart regression and fails the job. The post-test grep also
  flags `Tests leaked: [1-9]` so a leak-only failure can't slip
  past via the existing `Tests failed: [1-9]` guard.
- **README perf section.** Re-flowed to a single 1920×1080 resolution
  with one column each for SVG / PNG / WebP / JPG. SVG sits in the
  single-digit-ms range across families; PNG and JPG land in 60–85
  ms; WebP is the slowest encoder at 90–125 ms. Bench script
  consolidated to one `FC_BENCH_ITERS` knob.
- **v1 gallery (`docs/v1-gallery.html`)**: 4-up SVG / PNG / JPG /
  WebP layout, responsive at 720 / 1400 px breakpoints.
- **Build / CI deps cleaned.** Drop `libgd-dev` from the linux + macOS
  CI jobs (fastchart no longer links it; the ASAN job keeps it for
  the ext/gd built into the sanitized PHP). `scripts/pie-smoke.sh`
  rewritten for v1.0 — drops the removed `draw(\GdImage)` reference,
  asserts PNG / JPEG / WebP / SVG output via magic-byte checks.

### Removed (1.0.0 cleanup tail)

- Dead `renderGif()` / `renderAvif()` `ZEND_METHOD` bodies on `Chart`
  and `Symbol`. Never registered in the arginfo method tables, so
  PHP-land calls hit the engine's standard "undefined method" error
  anyway — the bodies were unreachable.
- `fastchart_gd_image_ce` NULL global and its extern declaration in
  `php_fastchart.h`. No code referenced it after the libgd link was
  removed.
- Stale `composer.json` description claiming charts are "drawn onto
  ext/gd `\GdImage` canvases via libgd".
- Stale `draw(\GdImage)` / `renderAvif` references in
  `fastchart.stub.php` docblocks.
- `tests/028_show_values.php` orphan helper. `fc_color_near()` is
  defined inline in every consumer `.phpt` now.

### Result

101 / 101 phpt tests pass with ext/gd loaded. CI runs under ASan with
`detect_leaks=1` and zero fastchart-side leaks; the ext/gd MINIT
persistent allocations are filtered via a tight suppressions file.
Tests added in this followup wave: 132 (open_basedir TOCTOU), 133
(XML control-byte sanitizer), 134 (MINFO + optional-libs surface),
135 (AreaChart gradient).

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
