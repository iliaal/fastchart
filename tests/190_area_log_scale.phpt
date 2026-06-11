--TEST--
AreaChart log Y-axis: overlay and band render, stacked and non-positive throw
--EXTENSIONS--
fastchart
--FILE--
<?php

/* The range pass clamped dmin to 0 before the log check, so the
 * dmin <= 0 guard fired for every input and SCALE_LOG threw even on
 * strictly-positive data. Pre-fix the first two cases throw. */

/* Overlay: renders, decade ticks 10 and 100 frame the data. */
$svg = (new FastChart\AreaChart(400, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSeries([['data' => [10, 20, 30, 25]]])
    ->setYAxisScale(FastChart\Chart::SCALE_LOG)
    ->renderSvg();
var_dump(strlen($svg) > 0);
var_dump((bool) preg_match('/>10</', $svg), (bool) preg_match('/>100</', $svg));

/* Band mode: the envelope polygon has no zero anchor, so log scale
 * is a natural fit. */
$svg = (new FastChart\AreaChart(400, 300))
    ->setStacked(false)
    ->setBandMode(true)
    ->setSeries([
        ['data' => [80, 90, 70]],
        ['data' => [10, 20, 15]],
    ])
    ->setYAxisScale(FastChart\Chart::SCALE_LOG)
    ->renderSvg();
var_dump(strlen($svg) > 0);

/* Stacked areas anchor at 0; log scale is rejected with an honest
 * message instead of blaming the (positive) data. */
try {
    (new FastChart\AreaChart(400, 300))
        ->setStacked(true)
        ->setSeries([['data' => [10, 20]], ['data' => [5, 5]]])
        ->setYAxisScale(FastChart\Chart::SCALE_LOG)
        ->renderSvg();
    echo "no error\n";
} catch (ValueError $e) {
    echo $e->getMessage(), "\n";
}

/* Non-positive data still rejected. */
try {
    (new FastChart\AreaChart(400, 300))
        ->setSeries([['data' => [0, 20, 30]]])
        ->setYAxisScale(FastChart\Chart::SCALE_LOG)
        ->renderSvg();
    echo "no error\n";
} catch (ValueError $e) {
    echo $e->getMessage(), "\n";
}

?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
FastChart\AreaChart::draw(): log Y-axis requires non-stacked data (stacked areas anchor at 0)
FastChart\AreaChart::draw(): log Y-axis requires strictly-positive data
