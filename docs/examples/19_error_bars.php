<?php
/* Error bars on Line and Scatter. Each entry is either a single
 * positive scalar (symmetric ±) or [lo, hi] (asymmetric). NaN /
 * negative entries skip the whisker for that point. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\LineChart(640, 360))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Measured signal with confidence interval')
    ->setSeries([
        ['data' => [12.3, 14.1, 13.5, 15.8, 17.2, 18.9, 20.1, 19.4]],
    ])
    ->setCategoryLabels(['t1', 't2', 't3', 't4', 't5', 't6', 't7', 't8'])
    ->setErrorBars([0.8, 0.7, 1.2, 0.9, [0.5, 1.5], 1.0, 0.6, 1.1])
    ->setMarkerStyle(FastChart\Chart::MARKER_CIRCLE)
    ->renderToFile(__DIR__ . '/19a_line_errors.png');

(new FastChart\ScatterChart(640, 360))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Scatter sample with asymmetric uncertainty')
    ->setPoints([
        [1.0, 10.5], [2.0, 12.8], [3.0, 11.4], [4.0, 14.1],
        [5.0, 13.7], [6.0, 15.9], [7.0, 14.2], [8.0, 17.1],
    ])
    ->setErrorBars([
        [0.5, 1.0], [0.4, 0.8], [0.6, 1.2], [0.3, 0.6],
        0.7, 0.9, [0.5, 1.5], 0.8,
    ])
    ->setMarkerStyle(FastChart\Chart::MARKER_DIAMOND)
    ->setMarkerSize(7)
    ->renderToFile(__DIR__ . '/19b_scatter_errors.png');
