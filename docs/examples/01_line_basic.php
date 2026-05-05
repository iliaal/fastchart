<?php
/* Simplest possible line chart: one series, axes auto-scale.
 * renderToFile picks the format from the file extension. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\LineChart(640, 320))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Daily active users')
    ->setSeries([['data' => [820, 940, 870, 1020, 1180, 1250, 1340]]])
    ->setCategoryLabels(['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'])
    ->renderToFile(__DIR__ . '/01_line_basic.png');
