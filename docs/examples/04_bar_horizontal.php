<?php
/* Horizontal bars with stacking and a value-range plot band that
 * marks the SLA threshold. Long category labels render comfortably
 * along the Y axis. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\BarChart(680, 380))
    ->setFontPath($font)
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setTitle('p95 latency by endpoint (ms)')
    ->setSeries([
        ['label' => 'Read',  'data' => [120, 85, 240, 180, 95, 310, 145]],
        ['label' => 'Write', 'data' => [180, 120, 380, 290, 140, 480, 220]],
    ])
    ->setCategoryLabels([
        'users', 'sessions', 'reports',
        'search', 'auth', 'exports', 'health',
    ])
    ->setStacked(true)
    ->addVerticalBand(0, 200, 0x4FB286, 96, 'SLA')
    ->addVerticalBand(200, 500, 0xE34A6F, 96, 'over SLA')
    ->renderToFile(__DIR__ . '/04_bar_horizontal.png');
