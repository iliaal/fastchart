--TEST--
AreaChart: multi-series stacks by default per the public API
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_4b7d1e63: fastchart_area_init_extras defaulted stacked=false while the
 * stub documents that multi-series stacks unless setStacked(false) is passed. */

$series = [
    ['data' => [10, 20, 30]],
    ['data' => [5, 15, 25]],
];

$default = (new FastChart\AreaChart(300, 200))
    ->setSeries($series)
    ->setFillOpacity(0)
    ->renderSvg();

$stacked = (new FastChart\AreaChart(300, 200))
    ->setSeries($series)
    ->setStacked(true)
    ->setFillOpacity(0)
    ->renderSvg();

$overlay = (new FastChart\AreaChart(300, 200))
    ->setSeries($series)
    ->setStacked(false)
    ->setFillOpacity(0)
    ->renderSvg();

echo "default_matches_stacked: ",
    ($default === $stacked ? 'yes' : 'no'), "\n";
echo "default_differs_overlay: ",
    ($default !== $overlay ? 'yes' : 'no'), "\n";

?>
--EXPECT--
default_matches_stacked: yes
default_differs_overlay: yes