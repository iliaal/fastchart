--TEST--
SurfaceChart: heatmap with configurable color ramp + cell value labels
--EXTENSIONS--
fastchart
--FILE--
<?php

// 4x6 grid of values; ramp blue (low) -> red (high).
$grid = [
    [1, 2, 3, 4, 5, 6],
    [2, 4, 6, 8, 10, 12],
    [3, 6, 9, 12, 15, 18],
    [4, 8, 12, 16, 20, 24],
];
$bytes = (new FastChart\SurfaceChart(600, 400))
    ->setGrid($grid)
    ->setColorRamp(0x0000FF, 0xFF0000)
    ->setTitle('Heatmap')
    ->renderPng();
echo "renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

$im = imagecreatefromstring($bytes);

// The lowest cell should be near pure blue, the highest near pure red.
// Sample the top-left cell (low value 1) and bottom-right (high 24).
// Allow some slop because the ramp interpolates. Both samples sit
// well inside the 4-row x 6-col grid layout the renderer chose.
$low_cell  = imagecolorat($im, 60, 50);
$high_cell = imagecolorat($im, 540, 340);
$lr = ($low_cell >> 16) & 0xFF;
$lb = $low_cell & 0xFF;
$hr = ($high_cell >> 16) & 0xFF;
$hb = $high_cell & 0xFF;
echo "low_is_bluish: ", ($lb > $lr ? "yes" : "no ($lr,$lb)"), "\n";
echo "high_is_reddish: ", ($hr > $hb ? "yes" : "no ($hr,$hb)"), "\n";

// Show cell values on a small grid.
$bytes2 = (new FastChart\SurfaceChart(400, 300))
    ->setGrid([[1, 2], [3, 4]])
    ->setShowCellValues(true, '%.0f')
    ->renderPng();
echo "with_values: ", strlen($bytes2) > 1024 ? "ok" : "bad", "\n";

// Empty grid throws.
try {
    (new FastChart\SurfaceChart(300, 200))->setGrid([])->renderPng();
    echo "empty: no throw\n";
} catch (\Error $e) {
    echo "empty: error ok\n";
}
?>
--EXPECT--
renders: ok
low_is_bluish: yes
high_is_reddish: yes
with_values: ok
empty: error ok
