# FastChart code review findings

Review scope: performance, security, code cleanliness, and compliance with
FastChart/PHP extension standards.

Verification run:

```sh
make -j$(nproc)
composer validate --strict
ASAN_OPTIONS=detect_leaks=0 \
TEST_PHP_EXECUTABLE=$HOME/php-install-PHP-8.4/bin/php \
TEST_PHP_ARGS="-d extension=$HOME/php-src-8.4/ext/gd/modules/gd.so -d extension=$PWD/modules/fastchart.so" \
NO_INTERACTION=1 \
$HOME/php-install-PHP-8.4/bin/php run-tests.php -q tests/
```

Result: build completed, `composer.json` validated, and 128/128 PHPT tests
passed in 19.587 seconds. Smoke load reported FastChart version `0.2.0`.

## Findings

### P2 / Medium: source-image caps can still decode oversized compressed inputs before rejection

Affected lines:

- `fastchart_axis.c:974`
- `fastchart_axis.c:985`
- `fastchart_axis.c:988`

`fastchart_load_source_image()` applies an 8 MiB byte cap, then sniffs
PNG/JPEG/GIF/WebP dimensions before calling libgd. If the format is not
recognized by `fastchart_sniff_image_dims()` but libgd can still decode it,
the code calls `gdImageCreateFromFile()` first and only checks decoded
dimensions afterward.

That leaves a resource-exhaustion gap: a small compressed input with huge
decoded dimensions can force libgd to allocate and decode before FastChart
rejects it. The separate `stat`/sniff/decode sequence is also raceable when
the path points at attacker-writable storage, so a file can pass preflight and
then change before the final decode.

Recommended fix: decode only formats whose dimensions were preflighted, or
read the file once into a bounded buffer and decode from that stable buffer.
If FastChart wants to keep support for additional libgd formats, add explicit
dimension sniffers before handing them to libgd.

### P2 / Medium: vendored `qrcodegen_*` symbols are exported

Affected lines:

- `config.m4:104`
- `vendor/qrcodegen/qrcodegen.c:28`

`nm -D --defined-only modules/fastchart.so` shows Nayuki symbols such as
`qrcodegen_encodeText`, `qrcodegen_encodeBinary`, and `qrcodegen_getModule`
exported from the PHP extension. The shared PHP extension convention for the
iliaal extensions says statically vendored C libraries should not leak public
vendor symbols from the extension shared object. Only `get_module` should need
to remain exported for PHP module loading.

This creates ABI/interposition risk if another loaded extension or library
exports a different `qrcodegen_*` implementation.

Recommended fix: compile wrapper/vendor code with hidden visibility and make
only the PHP module entry visible, or locally neutralize/export-control the
vendored QR encoder symbols for this bundled build. Verify with:

```sh
nm -D --defined-only modules/fastchart.so | rg 'qrcodegen|get_module'
```

Expected result: `get_module` remains visible; `qrcodegen_*` does not.

### P3 / Low: `Chart::renderSvg()` stub docs are stale

Affected lines:

- `fastchart.stub.php:533`
- `fastchart.stub.php:546`
- `fastchart.c:3984`

The stub still says SVG output is supported only on `FastChart\LineChart`, and
that other chart families raise an error. The implementation now dispatches
all 19 chart families through `dispatch_svg_render()`, and the test suite has
SVG coverage for every chart family plus `Code128` and `QrCode`.

Because `fastchart.stub.php` is the arginfo and API-documentation source of
truth, users and generated docs now describe the wrong public API.

Recommended fix: update the `renderSvg()` and `drawSvgFragment()` comments in
`fastchart.stub.php`, then regenerate `fastchart_arginfo.h` through the normal
stub workflow.

### P3 / Low: `zend_long` diagnostics still use `%ld` through `(long)` casts

Affected lines:

- `fastchart.c:4142`
- `fastchart.c:4158`

`fastchart_resolve_canvas_dims()` formats `dpi` with `%ld` after casting to
`long`. Project standards require `ZEND_LONG_FMT` for `zend_long`. On LLP64
platforms such as Windows, `long` is 32-bit while `zend_long` can be 64-bit, so
the cast can truncate before formatting.

The current `setDpi()` bounds make this low practical risk, but it is still a
standards violation in shared render helper code.

Recommended fix: use `ZEND_LONG_FMT` for `width`, `height`, and `dpi` where
they are `zend_long`, and pass the original `zend_long` values without casts.

### P3 / Low: AreaChart gradient fill has a bounded but expensive worst case

Affected lines:

- `fastchart_effects.c:153`
- `fastchart_effects.c:171`
- `fastchart_area.c:161`
- `fastchart_area.c:226`

`fastchart_gradient_filled_polygon()` allocates a mask image for each polygon,
fills the polygon into the mask, then scans the full polygon bounding box with
`gdImageGetPixel()` and `gdImageSetPixel()`. AreaChart calls this per series
for stacked and non-stacked fills.

The existing canvas and series caps bound the cost, but a large canvas with
many area series can still run hundreds of millions of pixel checks in one
render. This is a performance hotspot, not an unbounded memory-safety issue.

Recommended fix: add a fast path for polygons whose bounding box covers most
of the plot, reuse a per-render mask where possible, or implement a scanline
fill that computes horizontal spans without per-pixel mask reads.

### P3 / Low: Code128 carries unused text-font fields

Affected lines:

- `php_fastchart.h:963`
- `php_fastchart.h:964`
- `fastchart_symbol.c:67`
- `fastchart_code128.c:490`

`fastchart_code128_obj` stores `text_font_path` and `text_font_size`, and the
symbol lifecycle code initializes, releases, and addrefs them. The public stub
only exposes `Code128::setShowText()`, and rendering uses
`fastchart_default_font_path` directly.

This is dead state unless a text-font API is planned for the current surface.

Recommended fix: either add the intended Code128 text-font setters and use the
stored fields in the renderer, or remove the unused fields and lifecycle code.

## Reviewed non-findings

I did not find a critical or high direct memory-safety, format-string, or SVG
injection issue. The code has strong hardening around embedded NUL rejection,
format validation, render allocation caps, `open_basedir` rechecks, SVG
escaping, and clone ownership.

Protocol-relative image-map links are intentionally allowed by
`tests/076_round4_deferred.phpt`. Change that only if the intended policy is
same-origin or relative-only image-map links.
