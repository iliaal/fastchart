--TEST--
Huge, skewed and empty stroke geometry rasterizes without undefined behavior
--DESCRIPTION--
UBSAN_OPTIONS makes a sanitizer build abort on the first report, so a
signed overflow, NULL memcpy source, clz(0) or float-to-int overflow in
the vendored FreeType-derived code fails this test. Without a sanitizer
build it still checks the rendered pixels.
--EXTENSIONS--
fastchart
gd
--ENV--
UBSAN_OPTIONS=halt_on_error=1:abort_on_error=1:print_stacktrace=1
--INI--
asan.detect_leaks=0
--FILE--
<?php

function render(string $body)
{
    return imagecreatefromstring(FastChart\Chart::svgToPng(
        '<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64">'
        . $body . '</svg>'));
}

function alpha_at($im, int $x, int $y): int
{
    return (imagecolorat($im, $x, $y) >> 24) & 0x7F;
}

$circle = render('<circle cx="32" cy="32" r="3e38" stroke="black" stroke-width="3e38"/>');
echo 'circle: ', alpha_at($circle, 5, 5) === 0 ? "filled\n" : "NO\n";

$dash = render('<path d="M0 0 L64 64 L0 64 C 100 100 -100 100 0 0 Z" stroke="black" '
    . 'stroke-width="2" stroke-dasharray="1e-38 1e-38" stroke-dashoffset="1e38"/>'
    . '<circle cx="32" cy="32" r="30" stroke="red" stroke-dasharray="1e-38 1e-38"/>');
echo 'dash: ', $dash ? "rendered\n" : "NO\n";

$skew = render('<g transform="skewX(90)"><rect width="10" height="10"/>'
    . '<circle r="5" stroke="black" stroke-width="2"/>'
    . '<path d="M0 0 C 1 2 3 4 5 6 A 3 3 0 1 1 9 9 Z" stroke="red"/></g>');
echo 'skew: ', $skew ? "rendered\n" : "NO\n";

$pct = render('<circle r="1e38%" cx="50%" cy="50%" stroke="red"/>');
echo 'percent radius: ', $pct ? "rendered\n" : "NO\n";

$miter = render('<path d="M10 10 L54 12 L10 14" fill="none" stroke="black" '
    . 'stroke-width="1e30" stroke-miterlimit="1e30"/>');
echo 'huge width and miter: ', $miter ? "rendered\n" : "NO\n";

$arc = render('<path d="M0 0 A 1 1 0 0 1 1e39 0" stroke="black"/>');
echo 'infinite arc: ', $arc ? "rendered\n" : "NO\n";

?>
--EXPECT--
circle: filled
dash: rendered
skew: rendered
percent radius: rendered
huge width and miter: rendered
infinite arc: rendered
