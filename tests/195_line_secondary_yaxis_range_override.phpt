--TEST--
LineChart: setYAxisRange applies only to the primary axis when secondary Y is on
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_7c4e9a12: range_r was passed through the same y_min/y_max override as
 * range_l, clipping the right-axis series to the primary scale. */

$svg = (new FastChart\LineChart(700, 400))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSecondaryYAxis(true)
    ->setYAxisRange(0, 50)
    ->setSeries([
        ['data' => [10, 20, 30, 40], 'axis' => 'left'],
        ['data' => [100, 200, 300, 400], 'axis' => 'right'],
    ])
    ->renderSvg();

echo "right_axis_shows_400: ",
    (preg_match('/>400</', $svg) ? 'yes' : 'no'), "\n";
echo "left_axis_shows_50: ",
    (preg_match('/>50</', $svg) ? 'yes' : 'no'), "\n";

?>
--EXPECT--
right_axis_shows_400: yes
left_axis_shows_50: yes