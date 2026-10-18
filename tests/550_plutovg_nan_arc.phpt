--TEST--
svgToPng: an arc whose radii collapse the centre to NaN draws a straight line
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

function stroke_png(string $d): string
{
    return FastChart\Chart::svgToPng(
        '<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64">'
        . '<path d="' . $d . '" fill="none" stroke="#000" stroke-width="4"/>'
        . '</svg>');
}

function opaque(string $png, int $x, int $y): bool
{
    $im = imagecreatefromstring($png);
    return ((imagecolorat($im, $x, $y) >> 24) & 0x7f) === 0;
}

/* rx=3e38 overflows rx*rx and ry=1e-38 underflows ry*ry, so the arc
 * centre is NaN. The segment count derived from it used to be cast to
 * int from NaN (undefined; optimized builds emitted cubics until the
 * process ran out of memory, -O0 builds drew nothing). */
$line = stroke_png('M8 32 L56 32');
$nan  = stroke_png('M8 32 A 3e38 1e-38 0 1 1 56 32');
echo "nan_arc_is_line: ", ($nan === $line ? "ok" : "fail"), "\n";
echo "nan_arc_midpoint_painted: ", (opaque($nan, 32, 32) ? "ok" : "fail"), "\n";

$t = microtime(true);
$png = stroke_png('M0 0 A 3e38 1e-38 1 1 1 1 7');
echo "scan_input_bounded: ",
    (strlen($png) > 0 && microtime(true) - $t < 10 ? "ok" : "fail"), "\n";

/* An ordinary arc still renders as an arc: the semicircle from (8,32)
 * to (56,32) with r=24 passes through (32,8), not the chord midpoint. */
$arc = stroke_png('M8 32 A 24 24 0 0 1 56 32');
echo "arc_apex_painted: ", (opaque($arc, 32, 8) ? "ok" : "fail"), "\n";
echo "arc_chord_clear: ", (!opaque($arc, 32, 32) ? "ok" : "fail"), "\n";
?>
--EXPECT--
nan_arc_is_line: ok
nan_arc_midpoint_painted: ok
scan_input_bounded: ok
arc_apex_painted: ok
arc_chord_clear: ok
