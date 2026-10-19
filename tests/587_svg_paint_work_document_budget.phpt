--TEST--
svgToPng: blending work is budgeted per document by paint kind
--EXTENSIONS--
fastchart
--FILE--
<?php

/* 251 full-canvas rects, each filled and stroked with a radial gradient
 * at opacity 0.7: 256 elements, exactly the element x pixel budget at
 * 4096 px, and ~32 s of blending in a release build. The paint budget
 * charges each pass by the pixels it blends times the paint kind's
 * weight and refuses the document after ~25 such passes (13 rects). */
$rects = [];
for ($i = 0; $i < 251; $i++) {
    $rects[] = '<rect x="0" y="0" width="4096" height="4096" fill="url(#g)" '
        . 'stroke="url(#g)" stroke-width="16384" opacity="0.7"/>';
}
$svg = '<svg xmlns="http://www.w3.org/2000/svg" width="4096" height="4096">'
    . '<defs><radialGradient id="g" cx="0.3" cy="0.4" r="0.05" fx="0.31" '
    . 'fy="0.41" spreadMethod="reflect" gradientTransform="rotate(17) skewX(10)">'
    . '<stop offset="0" stop-color="#f00" stop-opacity="0.6"/>'
    . '<stop offset="1" stop-color="#00f" stop-opacity="0.4"/>'
    . '</radialGradient></defs>' . implode('', $rects) . '</svg>';

try {
    FastChart\Chart::svgToPng($svg);
    echo "rendered\n";
} catch (ValueError $e) {
    echo $e->getMessage(), "\n";
}

/* The same rects with an opaque solid fill and no stroke cost ~2 s and
 * still render: opaque solid pixels are the cheapest paint. */
$solid = str_replace(['fill="url(#g)" stroke="url(#g)" stroke-width="16384" opacity="0.7"'],
    ['fill="#336699"'], $svg);
$info = getimagesizefromstring(FastChart\Chart::svgToPng($solid));
echo "opaque solid: {$info[0]}x{$info[1]}\n";

/* Opaque solid paint at partial coverage takes the per-pixel blend path,
 * ~7x a fully covered pixel. 80 paths of half-pixel strips cover the
 * whole canvas at coverage 128 each; charged at full-coverage rates they
 * fitted the budget with room to spare. */
$strips = ['M0 0'];
for ($y = 0; $y < 4096; $y++) {
    $strips[] = 'H4096v.5H0v.5';
}
$strips = '<path d="' . implode('', $strips) . '"/>';
$svg = '<svg xmlns="http://www.w3.org/2000/svg" width="4096" height="4096">'
    . str_repeat($strips, 80) . '</svg>';
try {
    FastChart\Chart::svgToPng($svg);
    echo "partial coverage: rendered\n";
} catch (ValueError $e) {
    echo "partial coverage: ", $e->getMessage(), "\n";
}
?>
--EXPECT--
FastChart\Chart::svgToPng() SVG render work exceeds cap (paint coverage too large)
opaque solid: 4096x4096
partial coverage: FastChart\Chart::svgToPng() SVG render work exceeds cap (paint coverage too large)
