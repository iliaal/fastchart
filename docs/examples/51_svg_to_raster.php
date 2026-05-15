<?php
/* SVG -> raster via the static Chart::svgToPng/Jpeg/Webp() entry
 * points. Useful for:
 *   - Round-tripping renderSvg() output through plutovg + libpng /
 *     libjpeg-turbo / libwebp without external tools
 *   - Stitching multiple chart fragments via drawSvgFragment() into
 *     one outer SVG document, then rasterizing the result
 *   - Any SVG-bytes-to-raster conversion that fastchart can serve
 *     in-process (no ImageMagick / rsvg-convert / inkscape fork)
 *
 * Constraints (per docs/specs/svg-to-raster.md):
 *   - SVG must be well-formed XML with intrinsic width/height (or a
 *     viewBox); percentage dimensions are rejected
 *   - <text> elements render BLANK (plutovg has no text engine).
 *     Flatten text to <path> data first via setSvgTextMode(PATHS)
 *     when building SVG inside fastchart, or use Inkscape's
 *     "Object to Path" on external SVG
 *   - SVG input cap: 16 MB
 *   - Output cap: 4096 px per side, 16M total pixels
 *
 * Errors:
 *   - \ValueError on malformed input / out-of-range dims / quality
 *   - \Error on rasterizer or encoder failure (or when the optional
 *     codec lib is not compiled in) */

require __DIR__ . '/_bootstrap.php';

/* Step 1: build two charts, get fragments, stitch them. */
$left  = (new FastChart\LineChart(320, 240))
    ->setFontPath($font)
    ->setTitle('Revenue')
    ->setSeries([['data' => [120, 145, 168, 190, 215]]])
    ->drawSvgFragment();
$right = (new FastChart\BarChart(320, 240))
    ->setFontPath($font)
    ->setTitle('Cost')
    ->setSeries([['data' => [80, 95, 110, 125, 140]]])
    ->drawSvgFragment();

$W = 640; $H = 240;
$svg  = '<?xml version="1.0" encoding="UTF-8"?>' . "\n";
$svg .= '<svg xmlns="http://www.w3.org/2000/svg" width="' . $W . '" height="' . $H
      . '" viewBox="0 0 ' . $W . ' ' . $H . '">';
$svg .= '<rect width="100%" height="100%" fill="#FFFFFF"/>';
$svg .= $left;
$svg .= '<g transform="translate(320,0)">' . $right . '</g>';
$svg .= '</svg>';
file_put_contents(__DIR__ . '/51_stitched.svg', $svg);

/* Step 2: rasterize the stitched SVG through each format. */
file_put_contents(__DIR__ . '/51_stitched.png',
    FastChart\Chart::svgToPng($svg));
file_put_contents(__DIR__ . '/51_stitched.jpg',
    FastChart\Chart::svgToJpeg($svg, 88, 0xF8F8F8));
file_put_contents(__DIR__ . '/51_stitched.webp',
    FastChart\Chart::svgToWebp($svg, 90, FastChart\Chart::WEBP_LOSSLESS));

/* Step 3: report sizes for comparison. */
printf("%-8s %10s\n", 'format', 'bytes');
printf("%-8s %10s\n", '------', '-----');
foreach (['svg', 'png', 'jpg', 'webp'] as $fmt) {
    $path = __DIR__ . "/51_stitched.$fmt";
    if (is_file($path)) {
        printf("%-8s %10d\n", $fmt, filesize($path));
    }
}
