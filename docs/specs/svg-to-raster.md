# Spec: `Chart::svgToPng/Jpeg/Webp()` static rasterizer methods

**Status:** shipped 2026-05-15
**Author:** ilia

Implementation: `ZEND_METHOD(FastChart_Chart,
svgToPng/svgToJpeg/svgToWebp)` in `fastchart.c`, plus
`fastchart_svg_get_intrinsic_dims()` in `fastchart_rasterize.c`.
Tests: `tests/149_svg_to_raster.phpt`. Example:
`docs/examples/51_svg_to_raster.php`.

## Motivation

Every `renderPng()` / `renderJpeg()` / `renderWebp()` call already
builds an SVG document with the same code as `renderSvg()`, passes it
to `fastchart_rasterize_svg()` (plutosvg + plutovg), and encodes the
result with libpng, libjpeg-turbo, or libwebp. The rasterize and
encode steps are separate functions in `fastchart_rasterize.c` and
`fastchart_encoder.c`.

Callers who stitch `drawSvgFragment()` output into an outer SVG
document had no way back to raster bytes inside fastchart. They had
to install ImageMagick, shell out to `rsvg-convert` or
`inkscape --export-png` (a fork per call), or use a much slower
pure-PHP rasterizer. Three static methods expose the existing
pipeline for caller-supplied SVG.

## API

```php
namespace FastChart;

abstract class Chart {
    public static function svgToPng(string $svg): string {}
    public static function svgToJpeg(string $svg, int $quality = 88,
                                     int $bgRgb = 0xFFFFFF): string {}
    public static function svgToWebp(string $svg, int $quality = 90,
                                     int $mode = Chart::WEBP_DRAWING): string {}
}
```

Each method returns encoded raster bytes, like `renderPng()` /
`renderJpeg()` / `renderWebp()`. Malformed XML, out-of-range
dimensions, exceeded caps, and post-parse raster backend failure throw
`\ValueError`. Any other plutovg failure, encoder failure, or a codec
library (libpng, libjpeg-turbo, or libwebp) that is not compiled in
throws `\Error`.

`svgToJpeg()` takes `$bgRgb` (24-bit RGB, default white) because JPEG
has no alpha channel: transparent regions are composited over this
color before encoding. PNG and WebP keep alpha.

`svgToWebp()` takes the same `$mode` values as the instance-side
`setWebpMode()`, since the same trade-offs apply to caller-supplied
SVG.

### Why static methods on `Chart`

The operation has no per-instance state, so a static method fits, as
with `Chart::version()`. Three methods on an existing class add less
API surface than a new `FastChart\Rasterizer` or `FastChart\Svg`
class with its own constructor and setters.

### Why three methods instead of one with a format enum

`Chart::rasterizeSvg($svg, $format, $quality)` would make every call
site pass a format constant. `Chart::svgToPng($svg)` reads clearly
and matches the `renderPng/Jpeg/Webp` naming.

## Constraints

The stub docblocks document these, and the methods enforce them at
runtime.

### 1. SVG must be well-formed XML

The bytes go straight to `plutosvg_document_load_from_data()`.
Malformed XML, a missing `<svg>` root, or unsupported features
(filters, masks, scripting, foreign objects) fail to load and throw
`\ValueError`.

### 2. Output dimensions come from the SVG

The rasterizer reads `width` / `height` / `viewBox` from the root
`<svg>` element; callers can't override them. Percentage dimensions
(`width="100%"`) are rejected because fastchart has no containing
viewport.

### 3. `<text>` renders blank

plutovg has no text renderer. The chart pipeline avoids this by
flattening every `<text>` element to glyph `<path>` data through
FreeType (`SVG_TEXT_PATHS` mode) before rasterizing.

Raw `<text>` elements in caller-supplied SVG render blank: plutosvg
parses them but produces no glyph geometry. The methods don't scan
for `<text` and reject it, because a substring scan is brittle (it
matches `<textPath>` and hits false positives in CDATA, comments, and
attribute values). The stub docblocks state the limitation instead:

> **SVG `<text>` elements are not rendered.** Flatten text to
> `<path>` data before calling this method. The fastchart SVG
> builder does this automatically via `setSvgTextMode(SVG_TEXT_PATHS)`;
> external SVG tools can use `text-to-path` conversion (Inkscape's
> "Object to Path", Illustrator's "Create Outlines", etc.).

### 4. Output dimension cap

The `FC_IMAGE_MAX_DIM` and `FC_IMAGE_MAX_PIXELS` caps used by
`setBackgroundImage()` apply: 4096 px per side and 16M total pixels
(a 64 MB RGBA buffer at most). SVG that declares larger dimensions is
rejected before allocation.

### 5. Embedded `data:image/` URIs are rejected

plutosvg's `<image href="data:image/(png|jpg|jpeg);base64,...">`
loader (`vendor/plutosvg/source/plutosvg.c:2426`) decodes the
embedded raster inline through libpng/libjpeg, which bypasses the
output cap in constraint 4. A 10x10 root SVG carrying a 4097x4097
embedded PNG would allocate about 67 MB inside plutosvg before the
cap check sees the rasterized size.

The public `svgTo*()` methods reject any SVG containing `data:image/`
(case-insensitive). To composite raster content with a chart
fragment, decode the images separately. Instance-side chart rendering
differs: the trusted builder validates source files first and may
embed their validated bytes before calling the internal rasterizer
directly.

### 6. `<use>` elements are rejected

plutosvg's `<use href="#id">` renderer
(`vendor/plutosvg/source/plutosvg.c:2121`) expands the referenced
subtree inline. Its cycle detector compares element pointers along
the ancestor chain but doesn't count fan-out. A 1.4 KB SVG with 8
nested `<g>` levels, each holding ten `<use>` references to the next,
triggers about 10^8 shape renders and about 14 s of render time on
commodity hardware, the SVG equivalent of a billion-laughs attack.

A 256-tag source-count cap (commit bacedd1) was not enough: nesting
amplifies expansion independently of source count, so 71 source
`<use>` tags reach the worst case. The public methods reject any
`<use>` (case-insensitive substring scan with a tag-name boundary
check). This includes SVG from `renderSvg()` or `drawSvgFragment()`,
so image-bearing generated SVG can't go back through `svgTo*()`.

The trusted chart builder has a bounded exception in the instance
render pipeline. It emits repeated source images as at most 33 unit
`<image>` definitions (one background plus the 32-icon public cap),
each placed by flat, one-level `<use>` references. No definition
contains another `<use>`, so fan-out can't grow with depth. PlutoSVG
also caches the decoded surface on the referenced image element for
the lifetime of the parsed document. `renderPng()`, `renderJpeg()`,
and `renderWebp()` pass this validated output directly to the
internal rasterizer; validation of caller-supplied SVG is unchanged.

### 7. Input size limits

Input SVG is capped at 16 MB. The parser also rejects documents with
more than 65,536 elements, 262,144 attributes, or 256 nesting levels,
which bounds parser memory and recursion below the byte cap. SVG
whose element count times output pixels exceeds the render-work
budget is also rejected. All throw `\ValueError`.

## Out of scope

- DPI or scale override (for example, `svgToPng($svg, $scale = 2.0)`)
- Caller-specified output dimensions
- Native `<text>` rendering through FreeType outside the chart
  builder (would require parsing `font-family`, `font-size`,
  `font-weight`, `text-anchor`, and `transform`, then resolving fonts)
- Streaming output for very large SVGs
- AVIF output (no AVIF encoder since v1.0)
