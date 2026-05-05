<?php
/* 2D scalar field rendered two ways from the same row-major grid:
 *   - SurfaceChart: heatmap fill keyed off the color ramp
 *   - ContourChart: iso-contours at user-supplied levels, optionally
 *     filled
 * setColorRamp takes [low_rgb, high_rgb] and interpolates linearly. */

require __DIR__ . '/_bootstrap.php';

$rows = 16; $cols = 24;
$grid = [];
for ($r = 0; $r < $rows; $r++) {
    $row = [];
    for ($c = 0; $c < $cols; $c++) {
        $row[] = sin($c / 4.0) * cos($r / 3.0) * 10.0;
    }
    $grid[] = $row;
}

(new FastChart\SurfaceChart(560, 320))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Surface heatmap')
    ->setGrid($grid)
    ->setColorRamp(0x1F4E79, 0xE34A6F)
    ->setShowCellValues(false)
    ->renderToFile(__DIR__ . '/15a_surface.png');

(new FastChart\ContourChart(560, 320))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Filled contour at 6 levels')
    ->setGrid($grid)
    ->setLevels([-8, -4, -2, 0, 2, 4, 8])
    ->setFilled(true)
    ->setColorRamp(0x1F4E79, 0xE34A6F)
    ->renderToFile(__DIR__ . '/15b_contour.png');
