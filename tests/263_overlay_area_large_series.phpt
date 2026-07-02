--TEST--
addOverlaySeries('area'): fill polygon covers every point past the old 1024 cap
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_3444b260: the overlay area-fill polygon used a fixed 2048-slot
 * buffer — the top edge stopped at the first 1024 valid points while the
 * bottom edge walked in from the last category, closing a self-crossing
 * polygon over disjoint x-ranges. The buffer is now sized to the data. */

use FastChart\BarChart;

$n = 1500;
$vals = [];
for ($i = 0; $i < $n; $i++) $vals[] = 10 + ($i % 7);

$svg = (new BarChart(800, 400))
    ->setSeries($vals)
    ->addOverlaySeries('area', $vals, ['color' => 0x00AA00])
    ->renderSvg();

if (!preg_match_all('/<polygon points="([^"]*)"/', $svg, $m)) {
    echo "no polygon found\n";
    exit;
}
/* The overlay fill is the polygon with the most points. Each vertex is
 * "x,y"; 1500 top-edge + 1500 bottom-edge = 3000 vertices. */
$max_pts = 0;
foreach ($m[1] as $pts) {
    $count = substr_count($pts, ',');
    if ($count > $max_pts) $max_pts = $count;
}
echo "overlay polygon vertices: $max_pts\n";

?>
--EXPECT--
overlay polygon vertices: 3000
