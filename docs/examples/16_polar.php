<?php
/* Polar chart: each point is [angle_deg, radius]. Useful for wind
 * roses, antenna patterns, anything cyclic. Multi-series supported
 * up to 8. */

require __DIR__ . '/_bootstrap.php';

$series = [];
$pts_a = [];
$pts_b = [];
for ($a = 0; $a <= 360; $a += 15) {
    $pts_a[] = [$a, 4 + 3 * sin(deg2rad($a) * 2)];
    $pts_b[] = [$a, 3 + 2 * cos(deg2rad($a))];
}

(new FastChart\PolarChart(520, 520))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Antenna gain pattern')
    ->setSeries([
        ['label' => 'Antenna A', 'data' => $pts_a],
        ['label' => 'Antenna B', 'data' => $pts_b],
    ])
    ->setMaxRadius(8)
    ->setSeriesColors([0x4F86C6, 0xE34A6F])
    ->renderToFile(__DIR__ . '/16_polar.png');
