# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.2] - 2026-05-15

A build/packaging patch: the 1.0.1 Linux prebuilt binaries linked
against the build host's `libjpeg.so.8` (Ubuntu's libjpeg-turbo
soname) and failed to load on Debian-based PHP, including every
official `php:X.Y-cli` Docker image where Debian's libjpeg-turbo
ships as `libjpeg.so.62` instead. Debian provides no package that
satisfies `libjpeg.so.8`, so `pie install iliaal/fastchart` on a
Debian PHP fell back to source build.

### Fixed

- **Linux prebuilt binaries now portable across distros.** Each
  `.so` produced by `.github/workflows/release-linux.yml` statically
  links freetype + libpng + libjpeg-turbo + libwebp from
  `./configure`-time source builds (pinned versions in workflow
  env). `--with-fastchart-static-codecs=<prefix>` is the new
  `config.m4` flag that drives it: prepends the prefix to
  `PKG_CONFIG_PATH`, switches every pkg-config call to `--static`,
  and appends `-Wl,--exclude-libs=ALL` so the codec-archive symbols
  stay local in `fastchart.so`. Without `--exclude-libs`, loading
  `ext/gd` (dynamically linked against the system libjpeg) in the
  same process collides on `jpeg_CreateCompress` with two different
  `JPEG_LIB_VERSION` values and produces "encoder produced no
  output" on the first `renderJpeg()`.
- **`readelf -d fastchart.so` NEEDED entries** trimmed from
  `libjpeg.so.8`, `libfreetype.so.6`, `libpng16.so.16`,
  `libwebp.so.7`, `libz.so.1`, `libm`, `libc` → just `libz`, `libm`,
  `libc`, `ld-linux`. Works on Ubuntu, Debian, RHEL, anywhere glibc
  + zlib are present.
- **macOS prebuilts unaffected.** Mach-O resolves by install-name
  (absolute path baked at link time), not soname major, so the
  Ubuntu↔Debian issue doesn't manifest. The macOS path keeps
  dynamic linking against Homebrew kegs.

### Changed

- `release-linux.yml` gains a `workflow_dispatch` trigger so
  maintainers can validate the recipe end-to-end without cutting a
  release tag. The release-asset upload step is gated on
  `github.event_name == 'release'`; manual runs build + verify but
  do not upload.
- `release-linux.yml` adds a `Verify codec libs are static` step
  that fails the job if `libjpeg`, `libpng`, `libwebp`, or
  `libfreetype` appears in the produced `.so`'s NEEDED entries —
  catches link-flag regressions immediately.
- Static-built codec libs cached via `actions/cache@v4` keyed on
  the pinned lib versions (env: `FT_VERSION`, `PNG_VERSION`,
  `JPEG_VERSION`, `WEBP_VERSION`). First build ~4min; subsequent
  cache-hit builds skip the source build.

Trade-off: `.so` size grows from ~4MB → ~6.8MB on Linux. Acceptable
in exchange for cross-distro portability.

## [1.0.1] - 2026-05-15

This is the first release with prebuilt binaries on GitHub Releases.
No API changes — every change in this release is build / packaging /
ZTS-correctness.

### Added

- **Prebuilt binaries on GitHub Releases.** `composer.json` now declares
  `download-url-method: ["pre-packaged-binary", "composer-default"]`,
  so PIE prefers a prebuilt asset and falls back to source build when
  no asset matches the install target. Two new workflows attach
  binaries on release publish:
  - `.github/workflows/windows.yml` — Windows DLLs for PHP-8.3/8.4/8.5
    (NTS + TS, x64 + x86) via `php/php-windows-builder`. System
    deps (freetype, libpng, libjpeg-turbo, libwebp) come through the
    action's `libs:` input and resolve via the PHP-on-Windows SDK
    deps server.
  - `.github/workflows/release-linux.yml` — Linux x86_64
    (`ubuntu-24.04`), Linux arm64 (`ubuntu-24.04-arm`), and macOS arm64
    (`macos-14`) `.so` binaries for PHP-8.4 and 8.5 (NTS) via
    `php/pie-ext-binary-builder`. apt-get / brew install the four
    deps before the build.

  PHP-8.3 on Linux/macOS, macOS Intel, and Alpine musl users continue
  to source-build via PIE's composer-default fallback.
- **`config.w32`** — Windows build manifest mirroring `config.m4`.
  All 29 wrapper sources + 12 vendor sources (qrcodegen + plutovg +
  plutosvg), FreeType mandatory via `CHECK_LIB("freetype_a.lib;...")`
  + `CHECK_HEADER_ADD_INCLUDE("ft2build.h", ..., ..\deps\include\
  freetype2)`, libpng / libjpeg-turbo / libwebp optional with the
  same `HAVE_LIBxxx` defines the Unix side uses.

### Fixed

- **ZTS module globals not zero-initialised.** Under NTS the globals
  struct lives in BSS so the linker zero-fills it; under ZTS it is
  heap-allocated per thread by TSRM and the contents are whatever the
  heap had. On Linux ZTS that often happened to be zero (fresh page
  from anonymous mmap) and the bug stayed latent; on Windows x64 ZTS
  the random content looked like a live `FT_Library` handle, so
  `FT_Done_Face` / `FT_New_Face` dereferenced garbage and every
  `renderXxx()` call segfaulted at first use. Explicit
  `memset(fastchart_globals, 0, sizeof(*fastchart_globals))` in the
  new `PHP_GINIT_FUNCTION` closes both. Valgrind on Linux ZTS now
  reports zero errors from zero contexts (was 14 from 7).
- **TSRMLS cache uninitialised in dynamically-loaded ZTS modules.**
  Added `ZEND_TSRMLS_CACHE_DEFINE()` at the `COMPILE_DL_FASTCHART`
  level and `ZEND_TSRMLS_CACHE_UPDATE()` in `PHP_GINIT_FUNCTION`.
  Without these, every `FASTCHART_G(...)` dereference from a loaded
  DSO under ZTS reads an undefined `__declspec(thread)` slot —
  silent on Linux ZTS (GCC `__thread` has weaker linkage), segfault
  on Windows ZTS at first access. Mirrors what ext/intl, ext/mbstring,
  ext/curl have always done.
- **Default font path no longer a shared interned `zend_string`.**
  The interned-permanent shared string was a v1.0 round-2 fix for a
  refcount race under ZTS. After surfacing the deeper ZTS bug above
  it adds nothing, so it is gone. Each chart now `zend_string_init`s
  its own font_path from a `const char *` pointing into a static
  string-literal table. ~32 extra bytes per chart, no cross-thread
  refcount math.
- **Windows font candidates** for the default-font probe. The probe
  previously listed only `/usr/share`, `/Library`, and `/System/
  Library` paths; on Windows the probe returned NULL, every chart
  started with `font_path == NULL`, and every text-rendering call
  silently no-op'd. Added `C:\Windows\Fonts\arial.ttf` and
  `C:\Windows\Fonts\segoeui.ttf`.
- **MSVC C2057 in `fastchart_stock.c`.** `const int baseT = 10`
  followed by `int dq[baseT + 1]` is portable C99 (a VLA on GCC /
  Clang but with a compile-time-constant bound) — MSVC's C front-end
  rejects it as "expected constant expression". Converted `baseT` to
  a `#define` (scoped `#undef` at function end). All 17 other uses
  in arithmetic / bounds checks work unchanged with macro
  substitution.

### Changed

- **`tests/089_font_cache_open_basedir.phpt`** — moved the
  `/usr/share`-font probe into `--SKIPIF--`. The test was an early
  `echo "skip: ..."` + `exit;` inside `--FILE--`, which run-tests.php
  treated as a failed assertion (no `--SKIPIF--` block means "treat
  output literally"). Windows runners now skip cleanly instead of
  producing a confusing FAIL.
- **30 SVG-validating phpts declare `simplexml`** in `--EXTENSIONS--`.
  The tests parse SVG output with `simplexml_load_string()` to
  verify well-formedness and element counts. SimpleXML is normally
  compiled-in but on ZTS PHP builds where libxml2 / simplexml were
  dropped (custom `--disable-all` setups, ASAN-PHP builds) the tests
  fatal at the `simplexml_load_string` call. Adding it to
  `--EXTENSIONS--` makes run-tests.php skip with a clear "Required
  extension missing: simplexml" reason.

## [1.0.0] - 2026-05-15

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

- **Seven new chart families** lifting the family count from 19 to 26:
  - `BulletChart` — Stephen Few bullet: performance bar against
    qualitative bands with a target tick.
  - `ParetoChart` — descending bars + cumulative-percentage line
    overlay (the 80/20 visualization).
  - `CalendarHeatmap` — GitHub-style day-grid keyed by `YYYY-MM-DD`
    with a low/high color ramp.
  - `SunburstChart` — radial hierarchical donut; recursive `children`
    arrays with optional `value` per node (interior nodes auto-sum).
  - `SankeyChart` — bipartite / multi-layer flow with bezier ribbons;
    `setNodes()` + `setLinks()` with `from` / `to` indices.
  - `MarimekkoChart` — variable-width stacked columns where column
    width is proportional to category total.
  - `VectorChart` — arrow-on-grid vector field with magnitude scaling
    and optional ramp coloring.
- **`Funnel::STYLE_PYRAMID`**: opt into a triangle-with-bands layout
  instead of the default descending-trapezoid look. Value still
  drives shape — band heights are value-proportional, widths follow
  the triangle's natural taper.
- **`Chart::svgToPng() / svgToJpeg() / svgToWebp()`**: three static
  methods that hand caller-supplied SVG bytes to the same plutovg +
  encoder pipeline used by `renderPng()` / `renderJpeg()` /
  `renderWebp()`. Lets callers stitch chart fragments (via
  `drawSvgFragment()`) into one SVG document and round-trip the
  whole thing back to raster without ImageMagick / `rsvg-convert` /
  pure-PHP rasterizers. `svgToJpeg($svg, int $q = 88, int $bgRgb =
  0xFFFFFF)` composites transparent regions under `$bgRgb` before
  JPEG encode (JPEG has no alpha); `svgToWebp($svg, int $q = 90,
  int $mode = WEBP_DRAWING)` honours the same mode as the instance-
  side encoder. Hard caps: 16 MB input cap; output dims capped at
  4096 px / 16 Mpx; embedded `data:image/` URIs and `<use>` elements
  rejected (plutosvg's data-URI loader bypasses the output-dim cap,
  and its `<use>` renderer's cycle detector doesn't count fan-out,
  so a nested `<g><use/>×10` tree can hit billion-laughs expansion).
  Text in caller-supplied SVG renders blank — plutovg has no text
  renderer; flatten `<text>` to `<path>` upstream.
- **`Chart::setWebpMode(int)`** with class constants `WEBP_DRAWING`
  (default; encoder preset tuned for vector-like content),
  `WEBP_PHOTO` (preset for photo input), `WEBP_LOSSLESS` (lossless
  encode), and `WEBP_FAST` (lower-effort encode for batch jobs).
  Same setter on `Symbol`. Default `WEBP_DRAWING` matches what chart
  content actually is and produces visibly tighter file sizes than
  the libwebp simple-API default this release also drops.
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

### Followups landed (review-pass mediums + ZTS hardening)

- **UTF-8 sanitizer for SVG_TEXT_NATIVE.** The first sanitizer pass
  handled only single-byte C0 controls; bytes 0x80+ passed through
  verbatim. A lone continuation byte, a truncated 2/3/4-byte
  sequence, a UTF-8-encoded surrogate (`ED A0 80`), an overlong
  encoding (`C0 AF`), or any 5+-byte start (0xF8+) survived into
  `<text>` content and made xmllint reject the document. The
  hardened escaper decodes each multi-byte sequence and replaces
  ill-formed runs with U+FFFD; lone start bytes resync byte-by-byte
  rather than skipping the declared length of garbage. Valid
  multi-byte UTF-8 (2/3/4-byte, BMP + SMP) round-trips untouched.
- **FT_Face LRU cache.** FT_Library was already shared (round 2);
  FT_Face creation was still per-call. Each `FT_New_Face` parses
  the whole font file (tables, charmaps, glyph index) and
  dominated per-label cost on dense labels. New 4-slot LRU cache
  keyed by `font_path` skips `FT_New_Face` on a hit; `FT_Set_Char_-
  Size` still runs every call (cheap).
- **ZTS-safe FT state via TSRM module globals.** FT_Library + face
  cache are now per-thread under ZTS (`ZEND_DECLARE_MODULE_GLOBALS`,
  `PHP_GSHUTDOWN` for per-thread teardown). NTS unchanged. Closes
  the last cross-thread shared-state race surface.
- **Interned default_font_path.** `fastchart_default_font_path` was
  a refcounted persistent zend_string; per-chart `zend_string_copy`
  on it called the non-atomic `GC_ADDREF`. Under ZTS two threads
  constructing charts concurrently raced on the refcount. Now
  interned via `zend_string_init_interned(..., permanent=1)` —
  copy returns the same pointer without touching the refcount.
- **AreaChart gradient honors `setFillOpacity()`.** The gradient
  emitter forced opaque stops; non-stacked overlay layers with
  gradient on lost the translucency that solid-fill overlays got
  from `setAreaAlpha`. Alpha now composes into the high byte of
  the gradient stops; legacy callers (Bar / Pie passing bare
  24-bit RGB) still get opaque stops via a default-to-0xFF
  fallback in `fc_svg_emit_gradient_def`.
- **Source-image S_ISREG defense-in-depth.** The single-open
  source-image loader added in the open_basedir fix relied on the
  MIME sniff to reject non-regular inodes. `php_stream_stat()` +
  `S_ISREG` after open now rejects directories, FIFOs, /proc
  entries up-front; stream wrappers without a real stat backend
  (http, php://memory) fall through to the MIME gate.
- **MINFO row labels.** `libjpeg-turbo => libjpeg-turbo 2.1.2`
  reduced to `libjpeg => 2.1.2 (turbo)` — value carries just the
  version with a flavour suffix, matching ext/gd's "libJPEG
  Version" idiom.
- **PLUTOVG_VERSION_STRING / PLUTOSVG_VERSION_STRING.** Dropped
  local `FC_VER_STR2` / `FC_XSTR_VER` stringification macros that
  reinvented what the vendored headers already export.

Tests added: 136 (S_ISREG rejection of non-regular inputs), 137
(AreaChart gradient alpha). Test 133 extended with malformed-UTF-8
probes (lone continuation, truncated 2/3-byte, surrogate, overlong,
above-U+10FFFF, valid round-trip). 103 / 103 phpts pass.

### Followups landed (post-1.0 draft audit rounds)

After the first 1.0.0 draft of CHANGELOG was written, five audit
rounds against the new SVG and chart-family surface produced the
following fixes. None of these are user-visible API changes; they
harden behaviour the 1.0.0 surface already exposed.

- **WebP encoder ~2× faster on chart content.** Switched from
  libwebp's `WebPEncodeRGBA` simple API to the advanced API with
  `WebPConfigPreset(WEBP_PRESET_DRAWING)`, `method=2`, and
  `thread_level=1`. Default `setWebpMode()` is `WEBP_DRAWING`;
  callers who actually encode photographic content can opt into
  `WEBP_PHOTO`, lossless via `WEBP_LOSSLESS`, or low-effort
  batches via `WEBP_FAST`.
- **`renderToFile('*.jpg')` now honours `setJpegQuality()`.** The
  default `$quality` argument was 90 and the validation rejected 0,
  which made the "fall back to instance setting" path unreachable.
  Default is now 0 (sentinel for "use per-format default"); JPEG
  promotes 0 to `self->jpeg_quality`, WebP promotes 0 to 90.
  Explicit `[1, 100]` still validated, OOB still rejected. Same
  shape on `Symbol::renderToFile`.
- **`clone $chart` deep-copies `config`.** Previously the lifecycle
  clone `Z_TRY_ADDREF`'d the config zval, so both clones shared the
  same HashTable. Setters like `addTextAnnotation`,
  `addOverlaySeries`, `addHorizontalLine`, `addVerticalLine` then
  aborted the process (Zend hash assertion, exit=134) on the first
  clone-side mutation. Lifecycle clone now recursively deep-copies
  every nested array in `config`, so a clone-then-append on a chart
  that already had annotations/overlays works regardless of how
  deep the config tree goes.
- **`svgToPng/Jpeg/Webp()` rejects DoS-amplifying inputs.** SVGs
  carrying `data:image/...` data URIs bypass the output-dim cap
  (plutosvg's loader decodes the embedded raster inline before our
  cap check sees the rasterized dims). SVGs carrying `<use>`
  elements can hit billion-laughs-style fan-out because plutosvg's
  cycle detector checks ancestor pointers but not source-tag count.
  Both are rejected with `\ValueError` at the entry point.
- **Atomic mutation on setters that take collections.** Line / Area
  / Bar `setSeries`, Surface / Contour `setGrid`, and Sunburst
  `setHierarchy` now parse into a local temp first and swap into
  `self` only on success. Strict-mode failures and depth-cap
  rejections no longer leave the chart with a partial replacement
  and the original collection released.
- **`SunburstChart`**: depth capped at `FASTCHART_SUNBURST_MAX_DEPTH=32`.
  Unbounded recursion in `setHierarchy` was a stack-exhaustion
  vector with adversarial input.
- **`BoxPlot::setBoxes`** silently drops entries that violate
  `min ≤ q1 ≤ median ≤ q3 ≤ max`. Pre-fix, an unordered five-number
  summary produced negative-height SVG rects.
- **`SankeyChart`**: `setNodes` now clears the previously-parsed
  link table; stale `from`/`to` indices into a longer node array
  could index past the new node count.
- **`VectorChart`**: rejects vectors whose magnitude is non-finite
  (NaN / ±inf) before float-to-int cast UB.
- **PNG dimension field UB**. Background-image PNG dimensions
  parsed via signed shift, which is undefined for the high bit.
  Parsed via `uint32_t` now and rejected when above `INT_MAX`.
- **`GanttChart`**: dependency narrowing — `zend_long` bounded into
  `[0, INT_MAX]` before the `(int)` cast.
- **`CalendarHeatmap`**: per-month day validation with leap-year
  handling. Bad ISO dates (`2026-02-30`) are rejected at setter
  time instead of being parsed into garbage offsets.
- **README + spec polish.** PHP-side docblocks for the SVG-to-raster
  entry points spell out the input-size cap, output-dim cap, and
  rejected-construct list. `docs/specs/svg-to-raster.md` matches
  the implementation (parameter signatures, dimension caps,
  embedded-image and `<use>` rejection notes).

Tests added in this wave: 138 (Funnel pyramid), 139–145 (one per
new chart family), 146 (Sankey/Sunburst/Vector/Bullet fresh-eyes
fixes), 147 (PNG dim UB / Gantt narrowing / ISO date), 148 (WebP
modes), 149 (svgToPng/Jpeg/Webp round-trip), 150 (data:image URI
rejection, atomic setSeries, BoxPlot monotonicity), 151 (Sunburst
atomic, `<use>` rejection), 152 (clone deep-copy + renderToFile
JPEG quality). 118 / 118 phpts pass.

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

[Unreleased]: https://github.com/iliaal/fastchart/compare/1.0.2...HEAD
[1.0.2]: https://github.com/iliaal/fastchart/releases/tag/1.0.2
[1.0.1]: https://github.com/iliaal/fastchart/releases/tag/1.0.1
[1.0.0]: https://github.com/iliaal/fastchart/releases/tag/1.0.0
[0.2.0]: https://github.com/iliaal/fastchart/releases/tag/0.2.0
[0.1.1]: https://github.com/iliaal/fastchart/releases/tag/0.1.1
[0.1.0]: https://github.com/iliaal/fastchart/releases/tag/0.1.0
