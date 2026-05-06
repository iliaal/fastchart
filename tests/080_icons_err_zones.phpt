--TEST--
addIconAt + setErrorBars typed migration + setZones typed migration
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Build a tiny PNG icon on the fly so the test is self-contained. */
$icon_path = __DIR__ . "/__icon.png";
$icon = imagecreatetruecolor(16, 16);
imagefilledrectangle($icon, 0, 0, 15, 15, 0xFF0000);
imagepng($icon, $icon_path);

/* IconPlot on ScatterChart at data coords. */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\ScatterChart(400, 200))
    ->setPoints([[1.0, 10.0], [2.0, 20.0], [3.0, 15.0]])
    ->addIconAt(2.0, 20.0, $icon_path, 24, 24)
    ->draw($im);
echo "scatter_icon: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* IconPlot on LineChart at fractional category index. */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\LineChart(400, 200))
    ->setSeries([['data' => [10, 20, 15, 30, 25]]])
    ->addIconAt(2.0, 30.0, $icon_path)
    ->addIconAt(0.5, 15.0, $icon_path, 12, 12)
    ->draw($im);
echo "line_icon: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Missing icon file silently skipped. */
$im = imagecreatetruecolor(200, 100);
$out = (new FastChart\ScatterChart(200, 100))
    ->setPoints([[1.0, 10.0]])
    ->addIconAt(1.0, 10.0, "/nonexistent/path/that/does/not/exist.png")
    ->draw($im);
echo "missing_icon: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Validation: empty path. */
try {
    (new FastChart\ScatterChart)->addIconAt(0.0, 0.0, "");
    echo "empty_path: no throw\n";
} catch (\ValueError $e) {
    echo "empty_path: ValueError ok\n";
}

/* Validation: NUL in path. */
try {
    (new FastChart\ScatterChart)->addIconAt(0.0, 0.0, "ok\0bad");
    echo "nul_path: no throw\n";
} catch (\ValueError $e) {
    echo "nul_path: ValueError ok\n";
}

/* Validation: huge width. */
try {
    (new FastChart\ScatterChart)->addIconAt(0.0, 0.0, $icon_path, 5000);
    echo "huge_w: no throw\n";
} catch (\ValueError $e) {
    echo "huge_w: ValueError ok\n";
}

/* Cap at 32 icons. */
$chart = new FastChart\ScatterChart;
for ($i = 0; $i < 32; $i++) $chart->addIconAt($i, $i, $icon_path);
try {
    $chart->addIconAt(99.0, 99.0, $icon_path);
    echo "thirty_third: no throw\n";
} catch (\ValueError $e) {
    echo "thirty_third: ValueError ok\n";
}

/* setErrorBars typed: scalar (symmetric) and [lo, hi] (asymmetric)
 * mixed across points, including NaN-ish (negative scalar -> dropped). */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\LineChart(400, 200))
    ->setSeries([['data' => [10, 20, 30, 25, 18]]])
    ->setErrorBars([2.0, [3.0, 5.0], -1.0, [0.0, 4.0], 1.5])
    ->draw($im);
echo "line_err: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\ScatterChart(400, 200))
    ->setPoints([[1, 10], [2, 20], [3, 15]])
    ->setErrorBars([1.0, [2.0, 3.0], 1.5])
    ->draw($im);
echo "scatter_err: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* setZones typed: full rendering with custom colors. */
$im = imagecreatetruecolor(300, 200);
$out = (new FastChart\GaugeChart(300, 200))
    ->setRange(0.0, 100.0)
    ->setValue(72.0)
    ->setZones([
        ['from' => 0,  'to' => 33, 'color' => 0x00CC00],
        ['from' => 33, 'to' => 66, 'color' => 0xFFCC00],
        ['from' => 66, 'to' => 100, 'color' => 0xCC0000],
    ])
    ->draw($im);
echo "gauge_zones: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Empty zones list is benign — falls back to single-color sweep. */
$im = imagecreatetruecolor(200, 100);
$out = (new FastChart\GaugeChart(200, 100))
    ->setValue(50.0)
    ->setZones([])
    ->draw($im);
echo "empty_zones: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

@unlink($icon_path);
?>
--EXPECT--
scatter_icon: ok
line_icon: ok
missing_icon: ok
empty_path: ValueError ok
nul_path: ValueError ok
huge_w: ValueError ok
thirty_third: ValueError ok
line_err: ok
scatter_err: ok
gauge_zones: ok
empty_zones: ok
