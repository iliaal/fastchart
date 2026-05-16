<?php
/* PolarChart::setInterpolation(INTERP_SMOOTH): Catmull-Rom curves
 * through the polar points instead of straight segments. Markers
 * still anchor to the original data points. addVectors() overlays
 * arrow vectors anchored in the same (angle, radius) data space —
 * useful for wind / flow / phase diagrams. */

require __DIR__ . '/_bootstrap.php';

/* 12-point closed curve: smooth lobed shape. */
$points = [];
for ($i = 0; $i < 12; $i++) {
    $a = $i * 30;
    $r = 6 + 3 * sin($a * M_PI / 180 * 3);
    $points[] = [$a, $r];
}

(new FastChart\PolarChart(560, 560))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Smooth polar curve + 8-compass wind vectors')
    ->setSeries([['label' => 'envelope', 'data' => $points]])
    ->setMaxRadius(12)
    ->setFilled(true)
    ->setInterpolation(FastChart\Chart::INTERP_SMOOTH)
    ->addVectors([
        ['angle' =>   0, 'radius' => 0, 'angle_to' =>   0, 'radius_to' => 4.5],
        ['angle' =>  45, 'radius' => 0, 'angle_to' =>  45, 'radius_to' => 3.2],
        ['angle' =>  90, 'radius' => 0, 'angle_to' =>  90, 'radius_to' => 5.8],
        ['angle' => 135, 'radius' => 0, 'angle_to' => 135, 'radius_to' => 2.4],
        ['angle' => 180, 'radius' => 0, 'angle_to' => 180, 'radius_to' => 6.1],
        ['angle' => 225, 'radius' => 0, 'angle_to' => 225, 'radius_to' => 2.9],
        ['angle' => 270, 'radius' => 0, 'angle_to' => 270, 'radius_to' => 4.2],
        ['angle' => 315, 'radius' => 0, 'angle_to' => 315, 'radius_to' => 3.7],
    ])
    ->renderToFile(__DIR__ . '/54_polar_smooth_vectors.png');
