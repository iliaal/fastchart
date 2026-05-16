<?php
/* BubbleChart with log-10 Y axis. Useful when Y spans several
 * orders of magnitude (e.g. cost vs traffic across small + large
 * customers). Same setYAxisScale(SCALE_LOG) API as Line / Bar /
 * Area / Scatter / Stock — all Cartesian families share the gate.
 *
 * The chart errors if any Y is non-positive (log domain). The
 * X axis stays linear; switching X to log isn't supported yet. */

require __DIR__ . '/_bootstrap.php';

$points = [];
for ($i = 1; $i <= 18; $i++) {
    /* y grows roughly exponentially; size encodes a third measure. */
    $y = pow(2.2, $i / 2.0);
    $points[] = [$i, $y, 8 + $i * 1.3];
}

(new FastChart\BubbleChart(640, 400))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Cost vs throughput (log Y)')
    ->setXAxisTitle('Tier')
    ->setYAxisTitle('Monthly cost ($)')
    ->setPoints($points)
    ->setYAxisScale(FastChart\Chart::SCALE_LOG)
    ->renderToFile(__DIR__ . '/55_bubble_log_axis.png');
