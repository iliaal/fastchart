--TEST--
Polar / radar / contour renderers honor the full public setter cap
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Public setter caps are the contract; the prior renderer-local
 * stack arrays (POLAR=512, RADAR=32, CONTOUR-levels=16) silently
 * dropped the upper half of an at-cap input. Bumped to match the
 * public caps. Verify with a high-index probe: a chart with N
 * items must produce different bytes than one with N-1, so the
 * Nth item is actually drawn. */

/* Polar: 1024 [deg, r] points in one series; flip the last point's
 * r to large to force a measurable visual delta. */
$pts_a = [];
for ($i = 0; $i < 1024; $i++) {
    $pts_a[] = [$i * 0.35, 1.0 + ($i % 7) * 0.1];
}
$pts_b = $pts_a;
$pts_b[1023][1] = 100.0;
$a = (new FastChart\PolarChart(200, 200))->setSeries([['data' => $pts_a]])->renderPng();
$b = (new FastChart\PolarChart(200, 200))->setSeries([['data' => $pts_b]])->renderPng();
echo "polar_index_1023: ", ($a !== $b ? "rendered" : "dropped"), "\n";

/* Radar: 128 axes. Same shape — perturb axis 127 only. */
$series_a = [['data' => array_fill(0, 128, 1.0)]];
$series_b = [['data' => array_fill(0, 128, 1.0)]];
$series_b[0]['data'][127] = 5.0;
$ra = (new FastChart\RadarChart(220, 220))->setSeries($series_a)->renderPng();
$rb = (new FastChart\RadarChart(220, 220))->setSeries($series_b)->renderPng();
echo "radar_axis_127:   ", ($ra !== $rb ? "rendered" : "dropped"), "\n";

/* Contour: 32 levels. Grid range is [0, 30] (i+j over 16x16) so all
 * level values must sit inside that span to actually draw a line.
 * Use 32 values spread across [1, 29] and only differ at index 31. */
$grid = [];
for ($i = 0; $i < 16; $i++) {
    $row = [];
    for ($j = 0; $j < 16; $j++) {
        $row[] = $i + $j;
    }
    $grid[] = $row;
}
$lv_a = $lv_b = [];
for ($k = 0; $k < 32; $k++) {
    $lv_a[$k] = $lv_b[$k] = 1 + 28 * $k / 31;
}
$lv_b[31] = 5.0;  /* different value, still inside the grid range */
$ca = (new FastChart\ContourChart(220, 220))->setGrid($grid)->setLevels($lv_a)->renderPng();
$cb = (new FastChart\ContourChart(220, 220))->setGrid($grid)->setLevels($lv_b)->renderPng();
echo "contour_level_31: ", ($ca !== $cb ? "rendered" : "dropped"), "\n";
?>
--EXPECT--
polar_index_1023: rendered
radar_axis_127:   rendered
contour_level_31: rendered
