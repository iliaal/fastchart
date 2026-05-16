<?php
/* AreaChart::setBandMode(true): with exactly two series the chart
 * fills the envelope between them instead of each series down to
 * the baseline. Classic uses: confidence intervals around a
 * forecast, min/max ranges, observed-vs-predicted bands. */

require __DIR__ . '/_bootstrap.php';

/* Daily forecast for the next 30 days, with a ±1σ envelope around it. */
$days = 30;
$mean = [];
$upper = [];
$lower = [];
for ($i = 0; $i < $days; $i++) {
    $m = 100 + 4 * $i + 8 * sin($i / 3.0);
    $sigma = 6 + $i * 0.35;
    $mean[]  = round($m, 1);
    $upper[] = round($m + $sigma, 1);
    $lower[] = round($m - $sigma, 1);
}

(new FastChart\AreaChart(680, 360))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('30-day forecast with confidence band')
    ->setXAxisTitle('Day')
    ->setYAxisTitle('Value')
    ->setSeries([
        ['label' => 'upper bound', 'data' => $upper],
        ['label' => 'lower bound', 'data' => $lower],
    ])
    ->setBandMode(true)
    ->setFillOpacity(96)
    ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT)
    ->renderToFile(__DIR__ . '/53_area_band.png');
