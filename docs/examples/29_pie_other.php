<?php
/* setOtherThreshold(percent, label?) aggregates every slice below
 * the given percent of the total into a single "Other" slice. The
 * threshold is in percent (0..100), the optional label customizes
 * the aggregated slice name. Useful for long-tail breakdowns where
 * you don't want 30 sub-1% slivers crowding the legend. */

require __DIR__ . '/_bootstrap.php';

$slices = [
    'Chrome'     => 64,
    'Safari'     => 19,
    'Firefox'    => 6,
    'Edge'       => 4,
    'Opera'      => 1.8,
    'Samsung'    => 1.5,
    'UC'         => 0.9,
    'Yandex'     => 0.6,
    'Brave'      => 0.5,
    'DuckDuckGo' => 0.4,
    'Vivaldi'    => 0.3,
];

(new FastChart\PieChart(560, 400))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Browser share — without aggregation')
    ->setSlices($slices)
    ->setSliceLabelPosition(FastChart\Chart::LABEL_OUTSIDE)
    ->setSliceLabelFormat('%.0f%%')
    ->renderToFile(__DIR__ . '/29a_pie_no_threshold.png');

(new FastChart\PieChart(560, 400))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Browser share — slices < 2% rolled into "Other"')
    ->setSlices($slices)
    ->setOtherThreshold(2.0, 'Other')
    ->setSliceLabelPosition(FastChart\Chart::LABEL_OUTSIDE)
    ->setSliceLabelFormat('%.0f%%')
    ->renderToFile(__DIR__ . '/29b_pie_other.png');
