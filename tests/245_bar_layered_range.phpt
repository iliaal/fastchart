--TEST--
BarChart STACK_LAYER: the Y range uses the per-series extent, not the stacked sum
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_6760e99a: STACK_LAYER set the stacked flag before range computation, so
 * two layered 50+50 series got a 0..100 range and rendered at half height. The
 * range now uses per-series extent in layered mode. */

use FastChart\BarChart;
use FastChart\Chart;

function yticks(string $svg): string {
    preg_match_all('/>(\d+)</', $svg, $m);
    $t = array_values(array_unique($m[1]));
    sort($t, SORT_NUMERIC);
    return implode(',', $t);
}

$single = (new BarChart())->setSize(400, 300)
    ->setSvgTextMode(Chart::SVG_TEXT_NATIVE)
    ->setSeries([['name' => 'a', 'data' => [50]]])
    ->renderSvg();

$layered = (new BarChart())->setSize(400, 300)
    ->setSvgTextMode(Chart::SVG_TEXT_NATIVE)
    ->setStackMode(Chart::STACK_LAYER)
    ->setSeries([['name' => 'a', 'data' => [50]], ['name' => 'b', 'data' => [50]]])
    ->renderSvg();

/* Layered two-series range == single-series range (both 0..50), so the Y tick
 * labels match. A stacked-sum range would top out at 100. */
echo "layered_per_series_range: ", (yticks($single) === yticks($layered) ? 'yes' : 'no'), "\n";

?>
--EXPECT--
layered_per_series_range: yes
