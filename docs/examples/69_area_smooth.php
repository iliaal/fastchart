<?php
/* Smooth area: setLineInterpolation(INTERP_SMOOTH) reshapes the fill's
 * top boundary into a Catmull-Rom curve, the same curve a smooth line
 * would trace. The INTERP_STEP_* modes turn it into a staircase
 * instead. Stacked layers tile without gaps. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\AreaChart(640, 360))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Monthly active users (smoothed)')
    ->setCategoryLabels(['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug'])
    ->setLineInterpolation(FastChart\Chart::INTERP_SMOOTH)
    ->setSeries([
        ['label' => 'web',    'data' => [32, 48, 41, 60, 55, 72, 68, 80]],
        ['label' => 'mobile', 'data' => [18, 24, 30, 28, 39, 44, 52, 61]],
    ])
    ->renderToFile(__DIR__ . '/69_area_smooth.png');
