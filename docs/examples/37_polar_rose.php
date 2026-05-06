<?php
/* Polar rose: angular-bar variant of PolarChart. Each (angle,
 * radius) entry renders as a wedge from the centre to that radius;
 * the wedge's angular width runs to the next entry's angle. The
 * canonical use is a wind rose: bearing distribution where
 * direction matters as much as magnitude. */

require __DIR__ . '/_bootstrap.php';

/* 16-direction frequency rose. fastchart polar angles are math
 * angles (CCW from the right), so 0° = east, 90° = north, etc.
 * Two-lobe pattern centred on east and north-west. */
$bearings = [];
for ($i = 0; $i < 16; $i++) {
    $deg = $i * 22.5;
    $east = exp(-pow(($deg -   0) / 30, 2)) * 18;
    $nw   = exp(-pow(($deg - 135) / 30, 2)) *  9;
    /* Wrap-around: distance to 0 also applies via 360. */
    $east_w = exp(-pow(($deg - 360) / 30, 2)) * 18;
    $bearings[] = [$deg, 2 + $east + $east_w + $nw];
}

(new FastChart\PolarChart(540, 540))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setStyle(FastChart\PolarChart::STYLE_ROSE)
    ->setTitle('Wind frequency by bearing')
    ->setSeries([['data' => $bearings, 'color' => 0x2E86DE]])
    ->renderToFile(__DIR__ . '/37_polar_rose.png');
