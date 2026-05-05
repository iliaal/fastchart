<?php
/* Box plot with five-number summary per category plus per-category
 * outliers drawn as small open circles. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\BoxPlot(640, 400))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Response time distribution by service')
    ->setYAxisTitle('Latency (ms)')
    ->setBoxes([
        ['label' => 'auth',    'min' => 18, 'q1' => 32, 'median' => 45,
         'q3' => 58, 'max' => 95,  'outliers' => [120, 135]],
        ['label' => 'profile', 'min' => 42, 'q1' => 65, 'median' => 82,
         'q3' => 105, 'max' => 160, 'outliers' => [220]],
        ['label' => 'search',  'min' => 80, 'q1' => 110, 'median' => 145,
         'q3' => 195, 'max' => 320, 'outliers' => [410, 480, 510]],
        ['label' => 'orders',  'min' => 25, 'q1' => 38, 'median' => 52,
         'q3' => 71, 'max' => 105],
    ])
    ->renderToFile(__DIR__ . '/09_boxplot.png');
