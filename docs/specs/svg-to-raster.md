# Spec: `Chart::svgToPng/Jpeg/Webp()` static rasterizer methods

**Status:** shipped
**Tracking task:** #79 (spec), #80 (implementation)
**Author:** ilia
**Created:** 2026-05-15
**Shipped:** 2026-05-15

Implementation lives at `fastchart.c` `ZEND_METHOD(FastChart_Chart,
svgToPng/svgToJpeg/svgToWebp)` plus the
`fastchart_svg_get_intrinsic_dims()` helper in
`fastchart_rasterize.c`. Tests: `tests/149_svg_to_raster.phpt`.
Example: `docs/examples/51_svg_to_raster.php`.

Decisions made before implementation (see open questions below for
context):

1. Methods live on `FastChart\Chart` (not a new utility class).
2. SVG `<text>` elements render blank — silent + documented.
3. `svgToWebp()` accepts the same `$mode` parameter as the
   instance-side `setWebpMode()`.
4. `svgToJpeg()` accepts an additional `$bgRgb` parameter (default
   `0xFFFFFF`) and composites under the rasterized output before
   JPEG encode. JPEG has no alpha channel; without this, transparent
   SVG regions would render against a fixed white fallback.

## Motivation

fastchart already has a complete SVG-bytes-to-raster pipeline
internally. Every `renderPng()` / `renderJpeg()` / `renderWebp()`
call builds an SVG document via the same code that drives
`renderSvg()`, hands the bytes to `fastchart_rasterize_svg()`
(plutosvg + plutovg), then encodes through libpng / libjpeg-turbo /
libwebp. The rasterize+encode tail is already factored as separable
functions in `fastchart_rasterize.c` and `fastchart_encoder.c`.

Users who stitch chart fragments via `drawSvgFragment()` into an
outer SVG document currently can't get back to raster bytes through
fastchart. They have to:

- Install ImageMagick (heavy dependency, slower than plutovg)
- Shell out to `rsvg-convert` or `inkscape --export-png` (system
  dependency, fork overhead per call)
- Use a pure-PHP rasterizer (orders of magnitude slower)

Exposing the existing internal pipeline as three static methods
closes that gap with ~80 LOC of C plus stub, while staying inside
the chart-library positioning (the pipeline is the same one charts
use — we're just letting callers hand us pre-built SVG).

## API

Three static methods on `FastChart\Chart`, matching the
`Chart::version()` precedent:

```php
namespace FastChart;

class Chart {
    public static function svgToPng(string $svg): string {}
    public static function svgToJpeg(string $svg, int $quality = 88, int $bgRgb = 0xFFFFFF): string {}
    public static function svgToWebp(string $svg, int $quality = 90, int $mode = self::WEBP_DRAWING): string {}
}
```

`svgToJpeg` accepts a `$bgRgb` parameter (24-bit RGB, default white
`0xFFFFFF`) because JPEG has no alpha channel — transparent SVG
regions are composited under this color before encoding. PNG and
WebP preserve alpha natively and don't need the parameter.

Returns the encoded raster bytes (matching the existing
`renderPng()` / `renderJpeg()` / `renderWebp()` shape). Throws
`\ValueError` on malformed input, `\Error` on rasterizer failure or
when the optional codec lib (libpng / libjpeg-turbo / libwebp) is
not compiled in.

### Why static methods, not an instance class

Two reasons:

1. Symmetry with `Chart::version()` — the only other static method.
   The operation has no per-instance state to carry across calls,
   so a static is the right shape.
2. Lowest API surface: three methods on an existing class beats
   adding a new `FastChart\Rasterizer` class with constructors,
   setters, and a `render()` method.

### Why three methods, not one with a format enum

`Chart::rasterizeSvg($svg, $format, $quality)` would force callers
to thread a format constant through every call site. Three named
methods read better at the use site (`Chart::svgToPng($svg)` is
self-documenting) and match the existing `renderPng/Jpeg/Webp`
naming convention.

## Constraints

These are documented in the stub docblocks and enforced at runtime:

### 1. SVG must be well-formed XML

The SVG bytes are passed directly to `plutosvg_document_load_from_data()`.
Malformed XML, missing `<svg>` root, or unsupported feature use
(filters, masks, scripting, foreign objects) produces a load
failure, surfaced as `\ValueError`.

### 2. Output dimensions come from the SVG itself

The rasterizer reads `width` / `height` / `viewBox` from the root
`<svg>` element. Callers have no override knob in v1. SVG with
percentage dimensions (`width="100%"`) is rejected — fastchart
doesn't carry a containing viewport.

Future work could add a `$scale` parameter; deferred to keep the v1
surface narrow.

### 3. Text rendering: paths only

plutovg has no text renderer. Our chart-side pipeline sidesteps
this by flattening every `<text>` element to glyph `<path>` data
via FreeType (`SVG_TEXT_PATHS` mode) before rasterizing.

Caller-supplied SVG containing raw `<text>` elements will render
those elements as blank — plutosvg parses them but produces no
glyph geometry. The stub docblocks must say:

> **SVG `<text>` elements are not rendered.** Flatten text to
> `<path>` data before calling this method. The fastchart SVG
> builder does this automatically via `setSvgTextMode(SVG_TEXT_PATHS)`;
> external SVG tools can use `text-to-path` conversion (Inkscape's
> "Object to Path", Illustrator's "Create Outlines", etc.).

This constraint is loud and documented; users who hit blank text
will see the limitation in the docs.

Future work: implement an in-process SVG `<text>` → FreeType path
conversion. This would require parsing `font-family`, `font-size`,
`font-weight`, `text-anchor`, and `transform` attributes from the
SVG and resolving fonts via FreeType. Non-trivial; deferred.

### 4. Output dimension cap

Same `FC_IMAGE_MAX_DIM` and `FC_IMAGE_MAX_PIXELS` caps already
enforced by `setBackgroundImage()` apply: 4096 px on either axis,
~16M total pixels (~64 MB RGBA buffer worst case). A malformed or
adversarial SVG that declares huge dimensions is rejected before
allocation.

### 5. Embedded `data:image/` URIs are rejected

plutosvg's `<image href="data:image/(png|jpg|jpeg);base64,...">`
loader at `vendor/plutosvg/source/plutosvg.c:2426` decodes the
embedded raster inline via libpng/libjpeg, bypassing the output
dimension cap (constraint #4). A 10x10 root SVG carrying a
4097x4097 embedded PNG would allocate ~67 MB inside plutosvg
before our cap check sees the actual rasterized dims.

The entry point rejects any SVG that contains the substring
`data:image/` (case-insensitive). Callers who need to composite
raster content with a chart fragment should decode their images
separately. Added 2026-05-15 in commit f0c967a.

### 6. `<use>` elements are rejected

plutosvg's `<use href="#id">` renderer at
`vendor/plutosvg/source/plutosvg.c:2121` walks the referenced
subtree inline. Its cycle detector compares element pointers along
the ancestor chain but does NOT count fan-out. A 1.4 KB SVG
defining 8 nested `<g>` levels where each contains 10× `<use>`
of the next triggers ~10^8 shape renders and ~14 s of render time
on commodity hardware — a billion-laughs equivalent.

A source-count cap was tried (commit bacedd1, 256-tag limit) and
proved insufficient: nesting amplifies expansion independently of
source count, so 71 source `<use>` tags can hit the worst case.
The entry point now rejects ANY `<use>` occurrence (substring
scan with tag-name boundary check, case-insensitive). fastchart's
own SVG output never emits `<use>`; callers stitching multiple
fragments position content inline via `<g transform="...">`
instead. Added 2026-05-15 in commit (this round).

### 7. Size limits on input SVG

Hard cap at 16 MB SVG bytes to prevent DoS via plutosvg's parser.
Rejected with `\ValueError`.

## Implementation sketch

```c
ZEND_METHOD(FastChart_Chart, svgToPng)
{
    zend_string *svg;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(svg)
    ZEND_PARSE_PARAMETERS_END();

    /* Validate input size. */
    if (ZSTR_LEN(svg) == 0 || ZSTR_LEN(svg) > FC_SVG_MAX_BYTES) {
        zend_value_error("FastChart\\Chart::svgToPng() SVG is empty or > 16 MB");
        RETURN_THROWS();
    }

    /* Parse SVG to discover intrinsic width/height. */
    int w, h;
    if (fastchart_svg_get_intrinsic_dims(ZSTR_VAL(svg), ZSTR_LEN(svg), &w, &h) != 0) {
        zend_value_error("FastChart\\Chart::svgToPng() SVG has no intrinsic dimensions");
        RETURN_THROWS();
    }
    if (w <= 0 || h <= 0 || w > FC_IMAGE_MAX_DIM || h > FC_IMAGE_MAX_DIM
        || (long long)w * h > FC_IMAGE_MAX_PIXELS) {
        zend_value_error("FastChart\\Chart::svgToPng() output dimensions out of range");
        RETURN_THROWS();
    }

    /* Rasterize + encode. Existing internals; no new code. */
    fastchart_pixels_t pix;
    fastchart_pixels_init(&pix, w, h);
    pix.dpi = 96;
    if (fastchart_rasterize_svg(ZSTR_VAL(svg), ZSTR_LEN(svg), w, h, &pix) != 0) {
        fastchart_pixels_release(&pix);
        zend_throw_error(NULL, "FastChart: plutovg rasterization failed");
        RETURN_THROWS();
    }
    smart_str out = {0};
    int rc = fastchart_encode_png(&out, &pix);
    fastchart_pixels_release(&pix);
    if (rc == -2) {
        zend_throw_error(NULL, "FastChart: libpng not compiled in");
        RETURN_THROWS();
    }
    if (rc != 0 || !out.s) {
        smart_str_free(&out);
        zend_throw_error(NULL, "FastChart: PNG encoding failed");
        RETURN_THROWS();
    }
    smart_str_0(&out);
    RETURN_STR(out.s);
}
```

`svgToJpeg` and `svgToWebp` are the same shape with different
encoder calls + quality parameter.

The `fastchart_svg_get_intrinsic_dims()` helper is new C code (~30
LOC); everything else is existing internals.

## Test plan

One new phpt file:

- `tests/15x_svg_to_raster.phpt`
  - Build a chart, get `renderSvg()` bytes, round-trip through
    `svgToPng()`, verify the PNG signature
  - Same for `svgToJpeg()` / `svgToWebp()`
  - Hand-written SVG with explicit `width="640" height="480"`,
    confirm output dims via libpng header inspection
  - Malformed SVG → ValueError
  - Empty string → ValueError
  - SVG > 16 MB → ValueError (truncate or skip — building a 16 MB
    string in phpt is awkward; use a small cap behind FC_DEV define)
  - SVG with `<text>` elements: verify no crash, document the
    blank-text result via comment

## Out of scope (deferred)

- DPI / scale override (`svgToPng($svg, $scale = 2.0)`)
- Caller-specified output dimensions
- Native `<text>` rendering via FreeType outside the chart builder
- Streaming output for huge SVGs
- AVIF output (no AVIF encoder in v1.0)

## Open questions

1. Should `svgToPng()` be on `Chart` (as proposed) or on a new
   `FastChart\Svg` utility class? Symmetry argument: the operation
   has nothing to do with charts conceptually, just shares the
   internal pipeline. But adding a class to expose three static
   methods is API surface bloat. Lean: keep on `Chart`.

2. What's the right rejection behavior for SVG that contains
   `<text>` elements? Three options:
   - **Silent:** render blank text, let users discover via output
     inspection (current sketch)
   - **Loud:** scan the input string for `<text` and reject with a
     clear error if found
   - **Permissive:** accept everything, document the limitation in
     the method docblock

   Scanning is brittle (matches `<textPath>`, false positives in
   CDATA / comments / attribute values). Silent + documented is
   the cleanest balance. Could revisit if users file confused bug
   reports.

3. Should `svgToWebp()` accept the `$mode` parameter from
   `setWebpMode()`? Yes — same trade-offs apply to user-supplied
   SVG as to chart-built SVG. Default to `WEBP_DRAWING`.
