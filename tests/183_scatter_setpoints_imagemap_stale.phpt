--TEST--
ScatterChart::setPoints() invalidates image-map areas from a prior render
--EXTENSIONS--
fastchart
--FILE--
<?php

/* image_map_areas borrows href/tooltip pointers from the parsed
 * points (fastchart_scatter.c). Re-calling setPoints() frees those
 * strings; the area list must be reset or getImageMap() reads freed
 * memory. Pre-fix this test prints freed-heap garbage in the <map>
 * (or aborts under ASAN); post-fix the map is empty until the next
 * render. */

$c = new FastChart\ScatterChart(400, 300);
$c->setPoints([
    [1, 2, 'href' => '/first-target',  'tooltip' => 'first'],
    [3, 4, 'href' => '/second-target', 'tooltip' => 'second'],
]);
$c->renderSvg();

$map = $c->getImageMap();
var_dump(substr_count($map, '/first-target'));
var_dump(substr_count($map, '/second-target'));

/* Replace the data: the old hrefs are freed here. */
$c->setPoints([
    [5, 6, 'href' => '/third-target', 'tooltip' => 'third'],
]);

/* No render since the data swap: the map must be empty, not a view
 * of freed memory. */
var_dump($c->getImageMap());

/* After a fresh render the map regenerates from the new points. */
$c->renderSvg();
$map = $c->getImageMap();
var_dump(substr_count($map, '/third-target'));
var_dump(substr_count($map, '/first-target'));

?>
--EXPECT--
int(1)
int(1)
string(0) ""
int(1)
int(0)
