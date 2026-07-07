--TEST--
Axis tick labels: setYAxisRange(null,null,2.5) explicit interval keeps 1 decimal
--EXTENSIONS--
fastchart
--FILE--
<?php

/* The forced-interval path (setYAxisRange third arg) walks the range in
 * 2.5 steps from the auto min (2.0): ticks 2.0/4.5/7.0/9.5/12.0/14.5/17.0.
 * These half-integer ticks must render with one decimal; the old rule
 * dropped it, so 4.5 read "4", 9.5 read "10", etc. */

$svg = (new FastChart\LineChart(500, 400))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSeries([['data' => [3, 16, 7, 11]]])
    ->setCategoryLabels(['a', 'b', 'c', 'd'])
    ->setYAxisRange(null, null, 2.5)
    ->renderSvg();

echo "has 4.5:  ", strpos($svg, '>4.5<')  !== false ? "yes" : "no", "\n";
echo "has 9.5:  ", strpos($svg, '>9.5<')  !== false ? "yes" : "no", "\n";
echo "has 14.5: ", strpos($svg, '>14.5<') !== false ? "yes" : "no", "\n";

/* Buggy whole-number mislabels absent. */
echo "no 4 mislabel:  ", strpos($svg, '>4<')  === false ? "yes" : "no", "\n";
echo "no 14 mislabel: ", strpos($svg, '>14<') === false ? "yes" : "no", "\n";

?>
--EXPECT--
has 4.5:  yes
has 9.5:  yes
has 14.5: yes
no 4 mislabel:  yes
no 14 mislabel: yes
