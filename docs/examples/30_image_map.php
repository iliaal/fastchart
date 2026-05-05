<?php
/* Clickable scatter chart: each point can carry an `'href'` (and
 * optional `'tooltip'`) which getImageMap turns into HTML <area>
 * tags. The intended workflow:
 *   1. setPoints with href/tooltip per point
 *   2. draw / renderToFile produces the image
 *   3. getImageMap returns the <map> markup
 *   4. embed both in your HTML alongside <img usemap="#mapname">
 *
 * setXAxisLabelFormat applies a printf format to scatter X tick
 * labels (paired here so you can see the format in action). */

require __DIR__ . '/_bootstrap.php';

$chart = (new FastChart\ScatterChart(560, 360))
    ->setFontPath($font)
    ->setTitle('Quarterly revenue by region (clickable)')
    ->setXAxisTitle('Quarter')
    ->setYAxisTitle('Revenue ($M)')
    ->setXAxisLabelFormat('Q%.0f')
    ->setPoints([
        [1, 42, 'href' => '/dash?q=1&r=americas', 'tooltip' => 'Americas Q1'],
        [2, 51, 'href' => '/dash?q=2&r=americas', 'tooltip' => 'Americas Q2'],
        [3, 47, 'href' => '/dash?q=3&r=americas', 'tooltip' => 'Americas Q3'],
        [4, 63, 'href' => '/dash?q=4&r=americas', 'tooltip' => 'Americas Q4'],
        [1, 28, 'href' => '/dash?q=1&r=emea',     'tooltip' => 'EMEA Q1', 'color' => 0xE34A6F],
        [2, 33, 'href' => '/dash?q=2&r=emea',     'tooltip' => 'EMEA Q2', 'color' => 0xE34A6F],
        [3, 35, 'href' => '/dash?q=3&r=emea',     'tooltip' => 'EMEA Q3', 'color' => 0xE34A6F],
        [4, 39, 'href' => '/dash?q=4&r=emea',     'tooltip' => 'EMEA Q4', 'color' => 0xE34A6F],
    ])
    ->setMarkerStyle(FastChart\Chart::MARKER_CIRCLE)
    ->setMarkerSize(10);

$chart->renderToFile(__DIR__ . '/30_image_map.png');

/* getImageMap must run AFTER draw / render — the renderer is the one
 * that pixel-maps each point. Schemes outside http(s)/mailto/relative
 * paths are silently rejected, attribute values are HTML-escaped. */
$map = $chart->getImageMap('quarterly');

file_put_contents(__DIR__ . '/30_image_map.html', <<<HTML
<!doctype html>
<title>Image map demo</title>
<img src="30_image_map.png" usemap="#quarterly" alt="Quarterly revenue">
$map
HTML);

echo "Wrote 30_image_map.png and 30_image_map.html\n";
echo "Map has ", substr_count($map, '<area '), " clickable areas\n";
