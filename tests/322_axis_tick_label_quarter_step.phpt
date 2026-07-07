--TEST--
Axis tick labels: 0.25 step needs 2 decimals so 0.75 is not rounded to "0.8"
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Step 0.25 (ticks 0/0.25/0.5/0.75/1) needs two decimals. The old
 * log10-based guess gave one decimal, so 0.25 read "0.2" and 0.75 read
 * "0.8" while the gridlines sat at the true quarter positions. */

$svg = (new FastChart\LineChart(500, 400))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSeries([['data' => [0, 0.25, 0.5, 0.75, 1]]])
    ->setCategoryLabels(['a', 'b', 'c', 'd', 'e'])
    ->setYAxisRange(0, 1, 0.25)
    ->renderSvg();

echo "has 0.25: ", strpos($svg, '>0.25<') !== false ? "yes" : "no", "\n";
echo "has 0.75: ", strpos($svg, '>0.75<') !== false ? "yes" : "no", "\n";

/* The rounded-to-one-decimal mislabels must be gone. */
echo "no 0.2 mislabel: ", strpos($svg, '>0.2<') === false ? "yes" : "no", "\n";
echo "no 0.8 mislabel: ", strpos($svg, '>0.8<') === false ? "yes" : "no", "\n";

?>
--EXPECT--
has 0.25: yes
has 0.75: yes
no 0.2 mislabel: yes
no 0.8 mislabel: yes
