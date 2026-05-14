--TEST--
Per-chart-family raster round-trip: every family emits valid PNG/JPEG/WebP/SVG
--EXTENSIONS--
fastchart
gd
--FILE--
<?php
/* Coverage gap before this test: PNG was exercised by ~50 phpts but
 * JPEG and WebP only had 4 callsites total, and exclusively on
 * LineChart + Symbol classes. A chart family could break JPEG /
 * WebP encoding without anyone noticing. This test builds a minimal
 * instance of every chart family and asserts the four output
 * formats each produce magic-byte-correct bytes at non-trivial size. */

$lato = '/usr/share/fonts/truetype/lato/Lato-Regular.ttf';
$font = is_readable($lato) ? $lato
        : '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf';
if (!is_readable($font)) die("skip no system font available\n");

$ohlcv = [];
for ($i = 0; $i < 8; $i++) {
    $ohlcv[] = [1700000000 + $i * 86400,
                100 + $i, 102 + $i, 99 + $i, 101 + $i, 1000];
}

$families = [
    'LineChart'    => fn() => (new FastChart\LineChart(300, 200))
        ->setSeries([1, 2, 3, 4, 5]),
    'AreaChart'    => fn() => (new FastChart\AreaChart(300, 200))
        ->setSeries([1, 2, 3, 4, 5]),
    'BarChart'     => fn() => (new FastChart\BarChart(300, 200))
        ->setSeries([1, 5, 3]),
    'PieChart'     => fn() => (new FastChart\PieChart(300, 200))
        ->setSlices(['a' => 1, 'b' => 2, 'c' => 3]),
    'ScatterChart' => fn() => (new FastChart\ScatterChart(300, 200))
        ->setPoints([[1, 1], [2, 3], [3, 2]]),
    'StockChart'   => fn() => (new FastChart\StockChart(400, 250))
        ->setOhlcv($ohlcv),
    'RadarChart'   => fn() => (new FastChart\RadarChart(300, 300))
        ->setSeries([['data' => [3, 4, 5, 4, 3]]])
        ->setCategoryLabels(['a', 'b', 'c', 'd', 'e']),
    'BubbleChart'  => fn() => (new FastChart\BubbleChart(300, 200))
        ->setPoints([[1, 1, 10], [2, 3, 20], [3, 2, 15]]),
    'PolarChart'   => fn() => (new FastChart\PolarChart(300, 300))
        ->setSeries([['data' => [[0, 1], [45, 2], [90, 3]]]]),
    'SurfaceChart' => fn() => (new FastChart\SurfaceChart(300, 200))
        ->setGrid([[1, 2, 3], [4, 5, 6], [7, 8, 9]]),
    'ContourChart' => fn() => (new FastChart\ContourChart(300, 200))
        ->setGrid([[1, 2, 3], [4, 5, 6], [7, 8, 9]]),
    'GaugeChart'   => fn() => (new FastChart\GaugeChart(300, 200))
        ->setValue(42),
    'GanttChart'   => fn() => (new FastChart\GanttChart(400, 200))
        ->setTasks([
            ['label' => 't1', 'start' => 0, 'end' => 5],
            ['label' => 't2', 'start' => 3, 'end' => 8],
        ]),
    'BoxPlot'      => fn() => (new FastChart\BoxPlot(300, 200))
        ->setBoxes([['min' => 1, 'q1' => 2, 'median' => 3, 'q3' => 4, 'max' => 5]]),
    'Treemap'      => fn() => (new FastChart\Treemap(300, 200))
        ->setItems([['label' => 'a', 'value' => 5], ['label' => 'b', 'value' => 3]]),
    'Funnel'       => fn() => (new FastChart\Funnel(300, 200))
        ->setStages([['label' => 'a', 'value' => 100], ['label' => 'b', 'value' => 50]]),
    'Waterfall'    => fn() => (new FastChart\Waterfall(300, 200))
        ->setBars([['label' => 'a', 'value' => 100], ['label' => 'b', 'value' => -20]]),
    'Heatmap'      => fn() => (new FastChart\Heatmap(300, 200))
        ->setGrid([[1, 2], [3, 4]]),
    'LinearMeter'  => fn() => (new FastChart\LinearMeter(300, 60))
        ->setValue(40),
    'Code128'      => fn() => (new FastChart\Code128())
        ->setData('FC-12345')->setSize(300, 80),
    'QrCode'       => fn() => (new FastChart\QrCode())
        ->setData('https://example.com')->setSize(200, 200),
];

$png_magic  = "\x89PNG\r\n\x1a\n";
$jpeg_magic = "\xFF\xD8\xFF";
$webp_riff  = "RIFF";
$svg_prolog = '<?xml ';

$fail = 0;
foreach ($families as $name => $build) {
    $c = $build();
    /* Charts pick up the system font via setFontPath; Symbol classes
     * use a fixed monospace bar layout and don't need a font. */
    if (method_exists($c, 'setFontPath')) {
        $c->setFontPath($font);
    }

    $png  = $c->renderPng();
    $jpg  = $c->renderJpeg();
    $webp = $c->renderWebp();
    $svg  = $c->renderSvg();

    $row_ok = true;
    /* Size thresholds are intentionally low: Code128 with short data
     * encodes to ~450 bytes of PNG. Magic bytes are the primary
     * correctness check; size lower bound only catches "encoder
     * silently produced zero output". */
    if (substr($png, 0, 8) !== $png_magic || strlen($png) < 200) {
        echo "FAIL $name PNG: ", strlen($png), " bytes, head=", bin2hex(substr($png, 0, 4)), "\n";
        $row_ok = false;
    }
    if (substr($jpg, 0, 3) !== $jpeg_magic || strlen($jpg) < 200) {
        echo "FAIL $name JPEG: ", strlen($jpg), " bytes, head=", bin2hex(substr($jpg, 0, 4)), "\n";
        $row_ok = false;
    }
    if (substr($webp, 0, 4) !== $webp_riff
        || substr($webp, 8, 4) !== 'WEBP'
        || strlen($webp) < 100) {
        echo "FAIL $name WebP: ", strlen($webp), " bytes, head=", bin2hex(substr($webp, 0, 4)), "\n";
        $row_ok = false;
    }
    if (substr($svg, 0, 6) !== $svg_prolog || !str_contains($svg, '<svg ')) {
        echo "FAIL $name SVG: ", strlen($svg), " bytes\n";
        $row_ok = false;
    }
    if ($row_ok) {
        echo "$name: PNG/JPEG/WebP/SVG ok\n";
    } else {
        $fail++;
    }
}
echo $fail === 0 ? "ALL OK\n" : "FAILED $fail/" . count($families) . "\n";
?>
--EXPECT--
LineChart: PNG/JPEG/WebP/SVG ok
AreaChart: PNG/JPEG/WebP/SVG ok
BarChart: PNG/JPEG/WebP/SVG ok
PieChart: PNG/JPEG/WebP/SVG ok
ScatterChart: PNG/JPEG/WebP/SVG ok
StockChart: PNG/JPEG/WebP/SVG ok
RadarChart: PNG/JPEG/WebP/SVG ok
BubbleChart: PNG/JPEG/WebP/SVG ok
PolarChart: PNG/JPEG/WebP/SVG ok
SurfaceChart: PNG/JPEG/WebP/SVG ok
ContourChart: PNG/JPEG/WebP/SVG ok
GaugeChart: PNG/JPEG/WebP/SVG ok
GanttChart: PNG/JPEG/WebP/SVG ok
BoxPlot: PNG/JPEG/WebP/SVG ok
Treemap: PNG/JPEG/WebP/SVG ok
Funnel: PNG/JPEG/WebP/SVG ok
Waterfall: PNG/JPEG/WebP/SVG ok
Heatmap: PNG/JPEG/WebP/SVG ok
LinearMeter: PNG/JPEG/WebP/SVG ok
Code128: PNG/JPEG/WebP/SVG ok
QrCode: PNG/JPEG/WebP/SVG ok
ALL OK
