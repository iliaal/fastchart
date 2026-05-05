<?php
/* Radar / spider chart comparing two products across six criteria.
 * Axis labels come from setCategoryLabels. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\RadarChart(560, 480))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Feature parity scorecard')
    ->setSeries([
        ['label' => 'Product A', 'data' => [8, 7, 9, 6, 8, 7]],
        ['label' => 'Product B', 'data' => [6, 9, 7, 8, 5, 9]],
    ])
    ->setCategoryLabels(['Speed', 'Reliability', 'API', 'Docs', 'Support', 'Pricing'])
    ->setMaxValue(10)
    ->setSeriesColors([0x4F86C6, 0xE34A6F])
    ->renderToFile(__DIR__ . '/08_radar.png');
