--TEST--
BarChart: radial render resets image-map areas (no stale hot-spots)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: the radial render path never reset the image-map area
 * list (the vertical and horizontal paths do). A BarChart rendered with
 * setImageMap() active, then switched to BAR_RADIAL and re-rendered,
 * returned the stale rectangles from the prior layout. Radial bars are
 * arcs with no rect hot-spots, so a radial render must yield no areas. */

$c = (new FastChart\BarChart(400, 300))
    ->setSeries([10, 20, 30, 25, 15])
    ->setImageMap([
        ['href' => '/q/1'], ['href' => '/q/2'], ['href' => '/q/3'],
        ['href' => '/q/4'], ['href' => '/q/5'],
    ]);

$c->renderSvg();
echo "vertical_areas: ", count($c->getImageMapAreas()), "\n";

$c->setOrientation(FastChart\BarChart::BAR_RADIAL);
$c->renderSvg();
echo "radial_areas: ", count($c->getImageMapAreas()), "\n";

echo "ok\n";
?>
--EXPECT--
vertical_areas: 5
radial_areas: 0
ok
