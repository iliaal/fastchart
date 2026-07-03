--TEST--
ParetoChart honors a forced left-axis range via setYAxisRange()
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression (fnd_9f1a9070): ParetoChart computed its left (bar) axis
 * range without calling fastchart_value_range_apply_override, so the
 * inherited base setYAxisRange() was silently ignored. Pareto bars are
 * 0-based, so the max is the meaningful override. With a forced max of
 * 200 the top tick reads 200 and the bars scale to it; without it the
 * axis tops out near the data max (~50). */

$bars = [
    ['label' => 'A', 'value' => 50],
    ['label' => 'B', 'value' => 30],
    ['label' => 'C', 'value' => 20],
];

$forced = (new FastChart\ParetoChart(400, 300))
    ->setBars($bars)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setYAxisRange(0, 200)
    ->renderSvg();

$auto = (new FastChart\ParetoChart(400, 300))
    ->setBars($bars)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->renderSvg();

echo "forced axis shows 200: ", strpos($forced, '>200<') !== false ? "yes" : "no", "\n";
echo "auto axis omits 200:   ", strpos($auto, '>200<') === false ? "yes" : "no", "\n";

?>
--EXPECT--
forced axis shows 200: yes
auto axis omits 200:   yes
