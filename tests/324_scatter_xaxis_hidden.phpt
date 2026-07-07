--TEST--
ScatterChart honors setXAxisVisible(false): the X axis line/ticks are dropped
--EXTENSIONS--
fastchart
--FILE--
<?php

/* The hand-rolled scatter X axis never checked x_axis_visible, so
 * setXAxisVisible(false) was silently ignored. Delegating to the shared
 * numeric-axis drawer (which returns early when the axis is hidden)
 * suppresses the axis line, gridlines and tick marks; the Y axis and
 * markers stay. */

$pts = [[0, 0], [5, 10], [9, 3]];

$vis = (new FastChart\ScatterChart(400, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setPoints($pts)
    ->renderSvg();

$hidden = (new FastChart\ScatterChart(400, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setPoints($pts)
    ->setXAxisVisible(false)
    ->renderSvg();

$lv = substr_count($vis, '<line');
$lh = substr_count($hidden, '<line');

echo "hidden drops lines: ", $lh < $lv ? "yes" : "no", "\n";
/* Markers survive: the three points still render as circles. */
echo "markers survive: ", substr_count($hidden, '<circle') >= 3 ? "yes" : "no", "\n";

?>
--EXPECT--
hidden drops lines: yes
markers survive: yes
