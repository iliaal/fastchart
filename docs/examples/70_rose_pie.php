<?php
/* Variable-radius (rose) pie: each slice's angle still tracks its
 * value, but a positive "radius" key scales the slice's outer radius by
 * a second metric. Here every category has an equal value (equal angle)
 * so the varying radii carry all the contrast. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\PieChart(560, 460))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Regional revenue (radius = growth %)')
    ->setSlices([
        ['label' => 'North', 'value' => 20, 'radius' => 42, 'color' => 0x6C8EBF],
        ['label' => 'South', 'value' => 20, 'radius' => 88, 'color' => 0x82B366],
        ['label' => 'East',  'value' => 20, 'radius' => 64, 'color' => 0xD79B00],
        ['label' => 'West',  'value' => 20, 'radius' => 30, 'color' => 0xB85450],
        ['label' => 'Intl',  'value' => 20, 'radius' => 75, 'color' => 0x9673A6],
    ])
    ->renderToFile(__DIR__ . '/70_rose_pie.png');
