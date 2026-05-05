--TEST--
ContourChart: marching-squares isolines from a 2D grid
--EXTENSIONS--
fastchart
--FILE--
<?php

// A simple ramp. Default 5 evenly-spaced levels.
$grid = [
    [1, 2, 3, 4, 5],
    [2, 4, 6, 8, 10],
    [3, 6, 9, 12, 15],
    [4, 8, 12, 16, 20],
];
$bytes = (new FastChart\ContourChart(500, 400))
    ->setGrid($grid)
    ->setTitle('Heights')
    ->renderPng();
echo "auto_levels: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

// Explicit levels.
$bytes2 = (new FastChart\ContourChart(500, 400))
    ->setGrid($grid)
    ->setLevels([5.0, 10.0, 15.0])
    ->renderPng();
echo "explicit_levels: ", strlen($bytes2) > 1024 ? "ok" : "bad", "\n";

// Outline-only mode (no fill).
$bytes3 = (new FastChart\ContourChart(500, 400))
    ->setGrid($grid)
    ->setFilled(false)
    ->renderPng();
echo "outline: ", strlen($bytes3) > 1024 ? "ok" : "bad", "\n";

// Custom color ramp.
$bytes4 = (new FastChart\ContourChart(500, 400))
    ->setGrid($grid)
    ->setColorRamp(0x000000, 0xFFFFFF)
    ->renderPng();
echo "ramp: ", strlen($bytes4) > 1024 ? "ok" : "bad", "\n";

// Bad ramp color.
try {
    (new FastChart\ContourChart)->setColorRamp(-1, 0);
    echo "bad: no throw\n";
} catch (\ValueError $e) {
    echo "bad: ValueError ok\n";
}

// Too small grid.
try {
    (new FastChart\ContourChart(300, 200))->setGrid([[1, 2]])->renderPng();
    echo "small: no throw\n";
} catch (\Error $e) {
    echo "small: error ok\n";
}
?>
--EXPECT--
auto_levels: ok
explicit_levels: ok
outline: ok
ramp: ok
bad: ValueError ok
small: error ok
