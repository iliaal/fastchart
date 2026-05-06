--TEST--
addVerticalBand + IconPlot expand to remaining types + horizontal-bar polish
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Build a tiny PNG for icons. */
$icon_path = __DIR__ . "/__icon82.png";
$icon = imagecreatetruecolor(12, 12);
imagefilledrectangle($icon, 0, 0, 11, 11, 0x0080FF);
imagepng($icon, $icon_path);

/* === addVerticalBand on different X-axis kinds === */

/* Categorical X (LineChart): band spans category indices 1..3. */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\LineChart(400, 200))
    ->setSeries([['data' => [10, 20, 30, 25, 18]]])
    ->addVerticalBand(1.0, 3.0, 0xFFFF00, 96, "weekend")
    ->draw($im);
echo "v_band_categorical: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Data X (ScatterChart): band spans X = [2.5, 4.0]. */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\ScatterChart(400, 200))
    ->setPoints([[1.0, 10.0], [3.0, 20.0], [5.0, 15.0]])
    ->addVerticalBand(2.5, 4.0, 0x00FFFF, 96)
    ->draw($im);
echo "v_band_xrange: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Time X (StockChart): band spans a timestamp range. */
$rows = [];
$base = 1700000000;
for ($i = 0; $i < 10; $i++) {
    $p = 100 + $i;
    $rows[] = [$base + $i * 86400, $p, $p + 0.5, $p - 0.5, $p + 0.2, 1000];
}
$im = imagecreatetruecolor(500, 250);
$out = (new FastChart\StockChart(500, 250))
    ->setOhlcv($rows)
    ->addVerticalBand($base + 3 * 86400, $base + 6 * 86400, 0xFF8800, 100, "earnings")
    ->draw($im);
echo "v_band_time: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Mixed H + V bands on same chart. */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\LineChart(400, 200))
    ->setSeries([['data' => [10, 20, 30, 25, 18]]])
    ->addHorizontalBand(15, 25, 0x00FF00, 80)
    ->addVerticalBand(1.0, 3.0, 0xFFFF00, 96)
    ->draw($im);
echo "mixed_hv_bands: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Validation: bad color. */
try {
    (new FastChart\LineChart)->addVerticalBand(0.0, 1.0, 0x1000000);
    echo "vband_bad_color: no throw\n";
} catch (\ValueError $e) {
    echo "vband_bad_color: ValueError ok\n";
}

/* Validation: NaN. */
try {
    (new FastChart\LineChart)->addVerticalBand(NAN, 1.0, 0xFFFFFF);
    echo "vband_nan: no throw\n";
} catch (\ValueError $e) {
    echo "vband_nan: ValueError ok\n";
}

/* Cap is shared with horizontal bands at 16 total. */
$chart = new FastChart\LineChart;
for ($i = 0; $i < 8; $i++) $chart->addHorizontalBand($i, $i + 1, 0x808080);
for ($i = 0; $i < 8; $i++) $chart->addVerticalBand($i, $i + 1, 0x808080);
try {
    $chart->addVerticalBand(99.0, 100.0, 0x808080);
    echo "vband_seventeenth: no throw\n";
} catch (\ValueError $e) {
    echo "vband_seventeenth: ValueError ok\n";
}

/* === IconPlot on remaining chart types === */

/* AreaChart (categorical x). */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\AreaChart(400, 200))
    ->setSeries([['data' => [10, 20, 30, 25]]])
    ->addIconAt(2.0, 30.0, $icon_path)
    ->draw($im);
echo "area_icon: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* BarChart vertical (categorical x). */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\BarChart(400, 200))
    ->setSeries([['data' => [10, 20, 30, 25]]])
    ->addIconAt(1.0, 20.0, $icon_path)
    ->draw($im);
echo "bar_v_icon: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* BarChart horizontal (value x, categorical y). */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\BarChart(400, 200))
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setSeries([['data' => [10, 20, 30, 25]]])
    ->addIconAt(20.0, 1.0, $icon_path)
    ->draw($im);
echo "bar_h_icon: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* BoxPlot (categorical x). */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\BoxPlot(400, 200))
    ->setBoxes([
        ['min' => 1, 'q1' => 3, 'median' => 5, 'q3' => 7, 'max' => 9],
        ['min' => 2, 'q1' => 4, 'median' => 6, 'q3' => 8, 'max' => 10],
    ])
    ->addIconAt(0.5, 5.0, $icon_path)
    ->draw($im);
echo "box_icon: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* StockChart (timestamp x). */
$im = imagecreatetruecolor(500, 250);
$out = (new FastChart\StockChart(500, 250))
    ->setOhlcv($rows)
    ->addIconAt($base + 5 * 86400, 105.0, $icon_path)
    ->draw($im);
echo "stock_icon: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* === Horizontal-bar value labels === */

$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\BarChart(400, 200))
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setSeries([['data' => [10, -5, 25, 18]]])
    ->setShowValues(true)
    ->draw($im);
echo "h_bar_values: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* === Horizontal-bar value-axis bands === */

$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\BarChart(400, 200))
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setSeries([['data' => [10, 20, 30, 25]]])
    ->addVerticalBand(15.0, 25.0, 0xFFFF00, 96)
    ->draw($im);
echo "h_bar_v_band: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

@unlink($icon_path);
?>
--EXPECT--
v_band_categorical: ok
v_band_xrange: ok
v_band_time: ok
mixed_hv_bands: ok
vband_bad_color: ValueError ok
vband_nan: ValueError ok
vband_seventeenth: ValueError ok
area_icon: ok
bar_v_icon: ok
bar_h_icon: ok
box_icon: ok
stock_icon: ok
h_bar_values: ok
h_bar_v_band: ok
