--TEST--
ContourChart and SurfaceChart treat Inf / NaN cells as missing data
--EXTENSIONS--
fastchart
--FILE--
<?php

// Mixed grid with finite cells, +Inf, -Inf, NaN. Renderer must
// not crash, must not propagate non-finite into pixel coords or
// LUT indices, and must produce a sane image.
$grid = [
    [1.0,  2.0, INF],
    [3.0,  NAN, 5.0],
    [-INF, 7.0, 8.0],
];

$bytes = (new FastChart\ContourChart(400, 300))
    ->setGrid($grid)
    ->setColorRamp(0x0000FF, 0xFF0000)
    ->setFilled(true)
    ->renderPng();
echo "contour_renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

$bytes2 = (new FastChart\SurfaceChart(400, 300))
    ->setGrid($grid)
    ->setColorRamp(0x0000FF, 0xFF0000)
    ->renderPng();
echo "surface_renders: ", strlen($bytes2) > 1024 ? "ok" : "bad", "\n";

// All-non-finite grid: contour throws "no numeric values" because
// every cell is sanitized to NaN; surface raises the same shape.
try {
    (new FastChart\ContourChart(300, 200))
        ->setGrid([[INF, NAN], [NAN, -INF]])
        ->renderPng();
    echo "contour_all_bad: no throw\n";
} catch (\Error $e) {
    echo "contour_all_bad: error ok\n";
}

try {
    (new FastChart\SurfaceChart(300, 200))
        ->setGrid([[INF, NAN], [NAN, -INF]])
        ->renderPng();
    echo "surface_all_bad: no throw\n";
} catch (\Error $e) {
    echo "surface_all_bad: error ok\n";
}
?>
--EXPECT--
contour_renders: ok
surface_renders: ok
contour_all_bad: error ok
surface_all_bad: error ok
