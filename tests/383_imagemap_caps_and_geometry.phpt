--TEST--
Chart imagemap caps and hotspot geometry
--EXTENSIONS--
fastchart
--INI--
asan.detect_leaks=0
--FILE--
<?php

$old_map = [['href' => '/ok', 'tooltip' => 'kept']];
$c = (new FastChart\BarChart(300, 200))
    ->setSeries([1])
    ->setImageMap($old_map);
try {
    $c->setImageMap(array_fill(0, 4097, ['href' => '/too-many']));
    echo "entry cap: accepted\n";
} catch (ValueError $e) {
    echo "entry cap: valueerror\n";
}
$c->renderSvg();
$areas = $c->getImageMapAreas();
echo "old map kept: ", (count($areas) === 1 && $areas[0]['href'] === '/ok' ? 'yes' : 'no'), "\n";

try {
    $c->setImageMap([['href' => '/' . str_repeat('a', 4097)]]);
    echo "string cap: accepted\n";
} catch (ValueError $e) {
    echo "string cap: valueerror\n";
}

$sc = (new FastChart\ScatterChart(320, 260))
    ->setMarkerSize(10)
    ->setPoints([[1, 1, 'href' => '/p']]);
$sc->renderSvg();
$sc_area = $sc->getImageMapAreas()[0];
echo "scatter radius: ", $sc_area['coords'][2], "\n";

$pie = (new FastChart\PieChart(300, 300))
    ->setDonutHoleRatio(0.55)
    ->setSlices(['A' => 40, 'B' => 60])
    ->setImageMap([['href' => '/a'], ['href' => '/b']]);
$pie->renderSvg();
$pie_area = $pie->getImageMapAreas()[0];
echo "donut poly points: ", intdiv(count($pie_area['coords']), 2), "\n";
echo "donut poly annular: ", (count($pie_area['coords']) > 14 ? 'yes' : 'no'), "\n";

?>
--EXPECT--
entry cap: valueerror
old map kept: yes
string cap: valueerror
scatter radius: 5
donut poly points: 12
donut poly annular: yes
