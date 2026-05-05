<?php
/* Donut chart with one slice exploded outward to highlight it,
 * percentage labels positioned outside, and a custom palette. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\PieChart(560, 400))
    ->setFontPath($font)
    ->setTitle('Traffic source breakdown')
    ->setSlices([
        'Organic search' => 41,
        'Direct'         => 28,
        'Referral'       => 15,
        'Social'         => 9,
        'Email'          => 5,
        'Paid'           => 2,
    ])
    ->setDonutHoleRatio(0.5)
    ->setExplode([0 => 12])  // pull out the largest slice
    ->setSliceLabelPosition(FastChart\Chart::LABEL_OUTSIDE)
    ->setSliceLabelFormat('%.0f%%')
    ->setSeriesColors([0x4F86C6, 0xF6AE2D, 0xE34A6F, 0x4FB286, 0x9B5DE5, 0x9D9D9D])
    ->renderToFile(__DIR__ . '/05_pie_donut.png');
