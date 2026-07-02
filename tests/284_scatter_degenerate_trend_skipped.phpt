--TEST--
ScatterChart: a degenerate (all-same-x) linear trend is skipped, not drawn as y=0
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: with every point sharing one x the least-squares
 * denominator is zero; the code left coeffs at their zero defaults and
 * still ran the plot loop, drawing a bogus y=0 line. It now skips the
 * trend like the singular-matrix polynomial path. Observable: for
 * degenerate data, enabling the trend line changes nothing. */

$vertical = [[5, 1], [5, 5], [5, 9]];

$no_trend  = (new FastChart\ScatterChart(300, 200))
    ->setPoints($vertical)->setTrendLine(false)->renderSvg();
$yes_trend = (new FastChart\ScatterChart(300, 200))
    ->setPoints($vertical)->setTrendLine(true)->renderSvg();
echo "degenerate_no_extra_line: ", ($no_trend === $yes_trend ? "yes" : "no"), "\n";

/* Control: real (non-degenerate) data does get a trend line. */
$sloped = [[1, 1], [2, 2], [3, 3]];
$c_no  = (new FastChart\ScatterChart(300, 200))
    ->setPoints($sloped)->setTrendLine(false)->renderSvg();
$c_yes = (new FastChart\ScatterChart(300, 200))
    ->setPoints($sloped)->setTrendLine(true)->renderSvg();
echo "normal_draws_trend: ", ($c_no !== $c_yes ? "yes" : "no"), "\n";

?>
--EXPECT--
degenerate_no_extra_line: yes
normal_draws_trend: yes
