--TEST--
AreaChart: log Y-axis with secondary axis scales the right axis as log too
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_c4a8e2f1: range_r was only computed in the linear branch, so log scale +
 * setSecondaryYAxis(true) left range_r uninitialized before drawing the right
 * axis and mapping right-axis polygons.
 *
 * fnd_171a21f5: a log chart now maps the secondary (right) axis on a log scale
 * too rather than silently leaving it linear, so the right axis shows decade
 * ticks (100, 1000) for right data 100..400 instead of a "nice" 400 tick. */

$svg = (new FastChart\AreaChart(700, 400))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setStacked(false)
    ->setYAxisScale(FastChart\Chart::SCALE_LOG)
    ->setSecondaryYAxis(true)
    ->setSeries([
        ['data' => [10, 20, 30, 40], 'axis' => 'left'],
        ['data' => [100, 200, 300, 400], 'axis' => 'right'],
    ])
    ->renderSvg();

echo "right_axis_log_decade_1000: ",
    (preg_match('/>1000</', $svg) ? 'yes' : 'no'), "\n";
echo "left_log_tick_10: ",
    (preg_match('/>10</', $svg) ? 'yes' : 'no'), "\n";
echo "renders_polygon: ",
    (strpos($svg, '<polygon') !== false ? 'yes' : 'no'), "\n";

?>
--EXPECT--
right_axis_log_decade_1000: yes
left_log_tick_10: yes
renders_polygon: yes