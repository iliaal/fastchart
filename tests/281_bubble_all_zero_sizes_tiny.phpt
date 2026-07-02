--TEST--
BubbleChart: an all-zero size dimension renders as tiny markers, not medium
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: with no positive size the scale fraction fell back to 0.5,
 * so every zero-size bubble drew at half the maximum radius — reading as
 * real magnitude. The fallback is now 0.0, leaving only the rad<2 floor,
 * so zero-size points are tiny. */

function max_r(string $svg): int {
    preg_match_all('/\br="(\d+)"/', $svg, $m);
    return $m[1] ? max(array_map('intval', $m[1])) : -1;
}

$allzero = (new FastChart\BubbleChart(500, 400))
    ->setPoints([[1, 1, 0], [2, 2, 0], [3, 3, 0]])->renderSvg();
echo "allzero_max_r_small: ", (max_r($allzero) <= 2 ? "yes" : "no(" . max_r($allzero) . ")"), "\n";

/* A real size range still scales up to the large radius. */
$ranged = (new FastChart\BubbleChart(500, 400))
    ->setPoints([[1, 1, 0], [2, 2, 0], [3, 3, 10]])->renderSvg();
echo "ranged_has_large: ", (max_r($ranged) > 10 ? "yes" : "no(" . max_r($ranged) . ")"), "\n";

?>
--EXPECT--
allzero_max_r_small: yes
ranged_has_large: yes
