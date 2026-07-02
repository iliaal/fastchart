--TEST--
BarChart: radial orientation honors per-point color overrides
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: the radial bar path always used the series palette color,
 * while the vertical and horizontal paths resolve series[].colors via
 * bar_per_point_color(). A chart with per-point colors lost them the
 * moment it switched to radial. */

$svg = strtolower((new FastChart\BarChart(320, 320))
    ->setOrientation(FastChart\BarChart::BAR_RADIAL)
    ->setCategoryLabels(['a', 'b'])
    ->setSeries([[
        'name'   => 's',
        'data'   => [3, 5],
        'colors' => [0xFF0000, 0x00FF00],
    ]])
    ->renderSvg());

echo "has_red: ", (str_contains($svg, 'ff0000') ? "yes" : "no"), "\n";
echo "has_green: ", (str_contains($svg, '00ff00') ? "yes" : "no"), "\n";

?>
--EXPECT--
has_red: yes
has_green: yes
