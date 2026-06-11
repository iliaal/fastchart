--TEST--
BarChart: horizontal orientation emits image-map hot-spots
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_3568fad6: fastchart_bar_render_horizontal skipped image-map generation
 * that the vertical path performs after setImageMap(). */

$bar = (new FastChart\BarChart())
    ->setSize(400, 250)
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setSeries([10, 20, 30, 25, 15])
    ->setImageMap([
        ['href' => '/q/1', 'tooltip' => 'Q1: 10'],
        ['href' => '/q/2', 'tooltip' => 'Q2: 20'],
        ['href' => '/q/3', 'tooltip' => 'Q3: 30'],
        ['href' => '/q/4', 'tooltip' => 'Q4: 25'],
        ['href' => '/q/5'],
    ]);
$bar->renderSvg();
$map = $bar->getImageMap('hbars');

echo "areas: ", substr_count($map, '<area'), "\n";
echo "shape rect: ", strpos($map, 'shape="rect"') !== false ? 'ok' : 'BAD', "\n";
echo "tooltip present: ", strpos($map, 'title="Q3: 30"') !== false ? 'ok' : 'BAD', "\n";

$areas = $bar->getImageMapAreas();
echo "struct_count: ", count($areas), "\n";
echo "struct_shape: ",
    (isset($areas[0]['shape']) && $areas[0]['shape'] === 'rect') ? 'ok' : 'BAD', "\n";

?>
--EXPECT--
areas: 5
shape rect: ok
tooltip present: ok
struct_count: 5
struct_shape: ok