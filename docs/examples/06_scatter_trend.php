<?php
/* Scatter plot with a least-squares trend line. The trend overlay
 * uses degree=2 (quadratic) — pass 1 for a straight regression. */

require __DIR__ . '/_bootstrap.php';

$pts = [];
mt_srand(42);  // deterministic sample
for ($i = 0; $i < 60; $i++) {
    $x = $i * 0.5 + mt_rand(-100, 100) / 100.0;
    $y = 0.04 * $x * $x + 0.5 * $x + 5 + mt_rand(-200, 200) / 100.0;
    $pts[] = [$x, $y];
}

(new FastChart\ScatterChart(640, 400))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Page load time vs payload size')
    ->setXAxisTitle('Payload (KB)')
    ->setYAxisTitle('Time (s)')
    ->setPoints($pts)
    ->setMarkerStyle(FastChart\Chart::MARKER_CIRCLE)
    ->setMarkerSize(6)
    ->setTrendLine(true, 0xE34A6F, 2)  // quadratic fit, red
    ->renderToFile(__DIR__ . '/06_scatter_trend.png');
