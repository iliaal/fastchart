--TEST--
Chart::setImageMap() + getImageMap() across BarChart, PieChart, ScatterChart
--EXTENSIONS--
fastchart
--FILE--
<?php

/* BarChart: one rect hot-spot per category column. */
$bar = (new FastChart\BarChart())
    ->setSize(400, 250)
    ->setSeries([10, 20, 30, 25, 15])
    ->setImageMap([
        ['href' => '/q/1', 'tooltip' => 'Q1: 10'],
        ['href' => '/q/2', 'tooltip' => 'Q2: 20'],
        ['href' => '/q/3', 'tooltip' => 'Q3: 30'],
        ['href' => '/q/4', 'tooltip' => 'Q4: 25'],
        ['href' => '/q/5'],
    ]);
$bar->renderSvg();
$bar_map = $bar->getImageMap('bars');
echo "bar areas: ", substr_count($bar_map, '<area'), "\n";
echo "bar shape rect: ", strpos($bar_map, 'shape="rect"') !== false ? 'ok' : 'BAD', "\n";
echo "bar map name: ", strpos($bar_map, '<map name="bars">') !== false ? 'ok' : 'BAD', "\n";
echo "bar tooltip present: ", strpos($bar_map, 'title="Q3: 30"') !== false ? 'ok' : 'BAD', "\n";
echo "bar empty tooltip skipped: ", strpos($bar_map, 'title=""') === false ? 'ok' : 'BAD', "\n";

/* PieChart: poly hot-spot per slice. */
$pie = (new FastChart\PieChart())
    ->setSize(300, 300)
    ->setSlices(['A' => 30, 'B' => 50, 'C' => 20])
    ->setImageMap([
        ['href' => '/slice/a', 'tooltip' => 'A 30%'],
        ['href' => '/slice/b', 'tooltip' => 'B 50%'],
        ['href' => '/slice/c', 'tooltip' => 'C 20%'],
    ]);
$pie->renderSvg();
$pie_map = $pie->getImageMap('pie');
echo "pie areas: ", substr_count($pie_map, '<area'), "\n";
echo "pie shape poly: ", strpos($pie_map, 'shape="poly"') !== false ? 'ok' : 'BAD', "\n";

/* ScatterChart back-compat: per-point href/tooltip on setPoints
 * still works (no setImageMap needed for Scatter). */
$sc = (new FastChart\ScatterChart())
    ->setSize(300, 300)
    ->setPoints([
        [1, 1, 'href' => '/p1', 'tooltip' => 'one'],
        [2, 2, 'href' => '/p2'],
    ]);
$sc->renderSvg();
$sc_map = $sc->getImageMap();
echo "scatter areas: ", substr_count($sc_map, '<area'), "\n";
echo "scatter shape circle: ", strpos($sc_map, 'shape="circle"') !== false ? 'ok' : 'BAD', "\n";

/* Chart without setImageMap returns empty string. */
$empty = (new FastChart\BarChart())
    ->setSize(200, 100)
    ->setSeries([1, 2, 3]);
$empty->renderSvg();
echo "no-imap empty: ", $empty->getImageMap() === '' ? 'ok' : 'BAD', "\n";

/* Bad URL scheme rejected (javascript:). */
$mal = (new FastChart\BarChart())
    ->setSize(200, 100)
    ->setSeries([1, 2])
    ->setImageMap([
        ['href' => 'javascript:alert(1)'],
        ['href' => '/safe'],
    ]);
$mal->renderSvg();
$mal_map = $mal->getImageMap();
echo "js scheme rejected: ", strpos($mal_map, 'javascript:') === false ? 'ok' : 'BAD', "\n";
echo "safe url kept: ", strpos($mal_map, 'href="/safe"') !== false ? 'ok' : 'BAD', "\n";

/* Structured areas (P1) */
$bar_areas = $bar->getImageMapAreas();
echo "bar_struct_count: ", count($bar_areas), "\n";
echo "bar_struct_shape_str: ", (isset($bar_areas[0]['shape']) && $bar_areas[0]['shape']==='rect') ? 'ok' : 'BAD', "\n";
/* rect in HTML form: coords[2] should be x+w (not raw w) */
if (isset($bar_areas[0]['coords'][2])) {
    echo "bar_rect_html_form: ", ($bar_areas[0]['coords'][2] > $bar_areas[0]['coords'][0]) ? 'ok' : 'BAD', "\n";
}
echo "bar_struct_href_present: ", (isset($bar_areas[0]['href']) && strpos($bar_areas[0]['href'], '/q/') !== false) ? 'ok' : 'BAD', "\n";

$pie_areas = $pie->getImageMapAreas();
echo "pie_struct_count: ", count($pie_areas), "\n";
echo "pie_struct_shape_str: ", (isset($pie_areas[0]['shape']) && $pie_areas[0]['shape']==='poly') ? 'ok' : 'BAD', "\n";

$empty_areas = $empty->getImageMapAreas();
echo "no-imap-struct empty: ", (is_array($empty_areas) && count($empty_areas) === 0) ? 'ok' : 'BAD', "\n";
?>
--EXPECT--
bar areas: 5
bar shape rect: ok
bar map name: ok
bar tooltip present: ok
bar empty tooltip skipped: ok
pie areas: 3
pie shape poly: ok
scatter areas: 2
scatter shape circle: ok
no-imap empty: ok
js scheme rejected: ok
safe url kept: ok
bar_struct_count: 5
bar_struct_shape_str: ok
bar_rect_html_form: ok
bar_struct_href_present: ok
pie_struct_count: 3
pie_struct_shape_str: ok
no-imap-struct empty: ok
