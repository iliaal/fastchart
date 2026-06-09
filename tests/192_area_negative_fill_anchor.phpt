--TEST--
AreaChart: all-negative data keeps the zero baseline in the axis range
--EXTENSIONS--
fastchart
--FILE--
<?php

/* The non-stacked range scan zero-anchored only all-positive data;
 * all-negative data computed a range excluding 0, so the fill
 * baseline clamped to the plot top and the fill anchored at the axis
 * max instead of zero. Pre-fix the first render emits no 0 tick. */

/* All-negative overlay: the 0 tick must be on the axis. */
$svg = (new FastChart\AreaChart(400, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setCategoryLabels(['a', 'b', 'c'])
    ->setSeries([['data' => [-5, -3, -8]]])
    ->renderSvg();
var_dump((bool) preg_match('/>0</', $svg));
var_dump((bool) preg_match('/>-8</', $svg));

/* All-negative stacked: same anchor through the stacked range pass
 * (already held pre-fix via the dmin/dmax 0 seeds; pinned here so
 * the two passes stay symmetric). */
$svg = (new FastChart\AreaChart(400, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setCategoryLabels(['a', 'b', 'c'])
    ->setStacked(true)
    ->setSeries([['data' => [-5, -3]], ['data' => [-1, -2]]])
    ->renderSvg();
var_dump((bool) preg_match('/>0</', $svg));

/* All-positive behavior unchanged: zero stays anchored. */
$svg = (new FastChart\AreaChart(400, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setCategoryLabels(['a', 'b', 'c'])
    ->setSeries([['data' => [5, 3, 8]]])
    ->renderSvg();
var_dump((bool) preg_match('/>0</', $svg));

?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
