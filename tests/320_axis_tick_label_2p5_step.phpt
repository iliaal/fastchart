--TEST--
Axis tick labels: auto 2.5x10^N step keeps the fractional part (2.5/7.5/12.5)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* The nice-step generator emits a 2.5 step for everyday ranges (here
 * y 0..16 -> step 2.5, ticks 0/2.5/5/7.5/.../17.5). A "step >= 1 -> 0
 * decimals" rule labelled these as whole numbers (2.5->"2", 7.5->"8",
 * 17.5->"18") while the gridlines sat at the true fractional positions.
 * The label precision must follow the step's actual fractional part. */

$svg = (new FastChart\LineChart(500, 400))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSeries([['data' => [0, 16, 4, 12]]])
    ->setCategoryLabels(['a', 'b', 'c', 'd'])
    ->renderSvg();

echo "has 2.5:  ", strpos($svg, '>2.5<')  !== false ? "yes" : "no", "\n";
echo "has 7.5:  ", strpos($svg, '>7.5<')  !== false ? "yes" : "no", "\n";
echo "has 12.5: ", strpos($svg, '>12.5<') !== false ? "yes" : "no", "\n";
echo "has 17.5: ", strpos($svg, '>17.5<') !== false ? "yes" : "no", "\n";

/* The buggy whole-number mislabels of 7.5 and 17.5 must be gone. */
echo "no 8 mislabel:  ", strpos($svg, '>8<')  === false ? "yes" : "no", "\n";
echo "no 18 mislabel: ", strpos($svg, '>18<') === false ? "yes" : "no", "\n";

?>
--EXPECT--
has 2.5:  yes
has 7.5:  yes
has 12.5: yes
has 17.5: yes
no 8 mislabel:  yes
no 18 mislabel: yes
