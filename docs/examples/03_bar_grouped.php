<?php
/* Grouped (side-by-side) bars from multi-series input. Per-bar value
 * labels via setShowValues. Custom palette via setSeriesColors. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\BarChart(640, 380))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Monthly tickets opened vs closed')
    ->setSeries([
        ['label' => 'Opened', 'data' => [42, 38, 51, 47, 55, 49]],
        ['label' => 'Closed', 'data' => [35, 41, 48, 50, 52, 54]],
    ])
    ->setCategoryLabels(['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun'])
    ->setSeriesColors([0xE34A6F, 0x4FB286])
    ->setShowValues(true)
    ->setEdgeColor(0x222222)
    ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT)
    ->renderToFile(__DIR__ . '/03_bar_grouped.png');
