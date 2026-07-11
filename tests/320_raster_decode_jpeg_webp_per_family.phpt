--TEST--
Per-chart-family JPEG/WebP decode round-trip: every family's encoded bytes decode to the configured canvas
--EXTENSIONS--
fastchart
gd
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die("skip no system font available\n");
if (!function_exists('imagecreatefromjpeg')) die("skip gd built without jpeg support\n");
if (!function_exists('imagecreatefromwebp')) die("skip gd built without webp support\n");
?>
--FILE--
<?php
/* Test 131 checks only magic bytes + a length floor for JPEG/WebP.
 * Across the whole suite JPEG bytes are decoded in exactly one Symbol
 * test and WebP in one QrCode test, so no CHART family's JPEG or WebP
 * output was ever fed back through a decoder — an encoder that emitted
 * a valid header over a corrupt frame would pass 131 unnoticed. This
 * test builds the union of the 131 and 270 family sets (all 38 chart
 * classes plus both Symbol classes), decodes each family's JPEG and
 * WebP with imagecreatefromstring() (modern gd sniffs both), and
 * asserts the decoded image matches the configured canvas. */

require __DIR__ . '/_font_candidates.inc';
$font = fc_pick_font();
if ($font === '') die("skip no system font available\n");

$ohlcv = [];
for ($i = 0; $i < 8; $i++) {
    $ohlcv[] = [1700000000 + $i * 86400,
                100 + $i, 102 + $i, 99 + $i, 101 + $i, 1000];
}

/* Each entry: [width, height, builder]. Widths/heights mirror test 131
 * so the decoded dimensions can be asserted against the configured
 * canvas (raster size equals the canvas at the default 96 DPI). */
$families = [
    'LineChart'    => [300, 200, fn() => (new FastChart\LineChart(300, 200))
        ->setSeries([1, 2, 3, 4, 5])],
    'AreaChart'    => [300, 200, fn() => (new FastChart\AreaChart(300, 200))
        ->setSeries([1, 2, 3, 4, 5])],
    'BarChart'     => [300, 200, fn() => (new FastChart\BarChart(300, 200))
        ->setSeries([1, 5, 3])],
    'PieChart'     => [300, 200, fn() => (new FastChart\PieChart(300, 200))
        ->setSlices(['a' => 1, 'b' => 2, 'c' => 3])],
    'ScatterChart' => [300, 200, fn() => (new FastChart\ScatterChart(300, 200))
        ->setPoints([[1, 1], [2, 3], [3, 2]])],
    'StockChart'   => [400, 250, fn() => (new FastChart\StockChart(400, 250))
        ->setOhlcv($ohlcv)],
    'RadarChart'   => [300, 300, fn() => (new FastChart\RadarChart(300, 300))
        ->setSeries([['data' => [3, 4, 5, 4, 3]]])
        ->setCategoryLabels(['a', 'b', 'c', 'd', 'e'])],
    'BubbleChart'  => [300, 200, fn() => (new FastChart\BubbleChart(300, 200))
        ->setPoints([[1, 1, 10], [2, 3, 20], [3, 2, 15]])],
    'PolarChart'   => [300, 300, fn() => (new FastChart\PolarChart(300, 300))
        ->setSeries([['data' => [[0, 1], [45, 2], [90, 3]]]])],
    'SurfaceChart' => [300, 200, fn() => (new FastChart\SurfaceChart(300, 200))
        ->setGrid([[1, 2, 3], [4, 5, 6], [7, 8, 9]])],
    'ContourChart' => [300, 200, fn() => (new FastChart\ContourChart(300, 200))
        ->setGrid([[1, 2, 3], [4, 5, 6], [7, 8, 9]])],
    'GaugeChart'   => [300, 200, fn() => (new FastChart\GaugeChart(300, 200))
        ->setValue(42)],
    'GanttChart'   => [400, 200, fn() => (new FastChart\GanttChart(400, 200))
        ->setTasks([
            ['label' => 't1', 'start' => 0, 'end' => 5],
            ['label' => 't2', 'start' => 3, 'end' => 8],
        ])],
    'BoxPlot'      => [300, 200, fn() => (new FastChart\BoxPlot(300, 200))
        ->setBoxes([['min' => 1, 'q1' => 2, 'median' => 3, 'q3' => 4, 'max' => 5]])],
    'Treemap'      => [300, 200, fn() => (new FastChart\Treemap(300, 200))
        ->setItems([['label' => 'a', 'value' => 5], ['label' => 'b', 'value' => 3]])],
    'Funnel'       => [300, 200, fn() => (new FastChart\Funnel(300, 200))
        ->setStages([['label' => 'a', 'value' => 100], ['label' => 'b', 'value' => 50]])],
    'Waterfall'    => [300, 200, fn() => (new FastChart\Waterfall(300, 200))
        ->setBars([['label' => 'a', 'value' => 100], ['label' => 'b', 'value' => -20]])],
    'Heatmap'      => [300, 200, fn() => (new FastChart\Heatmap(300, 200))
        ->setGrid([[1, 2], [3, 4]])],
    'LinearMeter'  => [300, 60, fn() => (new FastChart\LinearMeter(300, 60))
        ->setValue(40)],
    'ArcDiagram'   => [400, 200, fn() => (new FastChart\ArcDiagram(400, 200))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 1],
                    ['from' => 1, 'to' => 2, 'value' => 2]])],
    'ChordDiagram' => [300, 300, fn() => (new FastChart\ChordDiagram(300, 300))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 1],
                    ['from' => 1, 'to' => 2, 'value' => 2]])],
    'NetworkChart' => [300, 300, fn() => (new FastChart\NetworkChart(300, 300))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 1],
                    ['from' => 1, 'to' => 2, 'value' => 2]])],
    'PopulationPyramid' => [300, 300, fn() => (new FastChart\PopulationPyramid(300, 300))
        ->setCategories(['a', 'b', 'c'])
        ->setLeftSeries(['label' => 'L', 'data' => [1, 2, 3]])
        ->setRightSeries(['label' => 'R', 'data' => [2, 3, 1]])],
    'ViolinPlot'   => [300, 300, fn() => (new FastChart\ViolinPlot(300, 300))
        ->setGroups([['label' => 'X', 'values' => [1, 2, 3, 4, 3, 2]]])],
    'CirclePacking' => [300, 300, fn() => (new FastChart\CirclePacking(300, 300))
        ->setHierarchy(['children' => [['value' => 5], ['value' => 3], ['value' => 8]]])],
    'Pictogram'    => [300, 150, fn() => (new FastChart\Pictogram(300, 150))
        ->setTotal(10)->setValue(6)],
    'VennDiagram'  => [300, 300, fn() => (new FastChart\VennDiagram(300, 300))
        ->setSets([['size' => 10], ['size' => 8]])
        ->setIntersections([['sets' => [0, 1], 'size' => 3]])],
    'WordCloud'    => [300, 300, fn() => (new FastChart\WordCloud(300, 300))
        ->setWords([['text' => 'alpha', 'weight' => 5],
                    ['text' => 'beta', 'weight' => 3],
                    ['text' => 'gamma', 'weight' => 8]])],
    'SerpentineTimeline' => [400, 200, fn() => (new FastChart\SerpentineTimeline(400, 200))
        ->setEvents([['label' => 'a', 'date' => 'Jan'],
                     ['label' => 'b', 'date' => 'Feb'],
                     ['label' => 'c', 'date' => 'Mar']])],
    'Dendrogram'   => [300, 300, fn() => (new FastChart\Dendrogram(300, 300))
        ->setHierarchy(['children' => [['value' => 5], ['value' => 3], ['value' => 8]]])],
    'Partition'    => [300, 300, fn() => (new FastChart\Partition(300, 300))
        ->setHierarchy(['children' => [['value' => 5], ['value' => 3], ['value' => 8]]])],
    'BulletChart'  => [400, 80, fn() => (new FastChart\BulletChart(400, 80))
        ->setRange(0, 100)
        ->setBands([
            ['from' => 0,  'to' => 60],
            ['from' => 60, 'to' => 85],
            ['from' => 85, 'to' => 100],
        ])
        ->setValue(72)->setTarget(80)],
    'ParetoChart'  => [400, 300, fn() => (new FastChart\ParetoChart(400, 300))
        ->setBars([
            ['label' => 'a', 'value' => 40],
            ['label' => 'b', 'value' => 30],
            ['label' => 'c', 'value' => 10],
        ])],
    'CalendarHeatmap' => [600, 140, fn() => (new FastChart\CalendarHeatmap(600, 140))
        ->setData(['2026-01-05' => 3, '2026-02-14' => 9, '2026-03-15' => 5])
        ->setColorRamp(0xEEFFEE, 0x004400)],
    'SunburstChart' => [300, 300, fn() => (new FastChart\SunburstChart(300, 300))
        ->setHierarchy([
            'label' => 'root',
            'children' => [
                ['label' => 'A', 'value' => 10],
                ['label' => 'B', 'value' => 20],
            ],
        ])],
    'SankeyChart'  => [400, 250, fn() => (new FastChart\SankeyChart(400, 250))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([
            ['from' => 0, 'to' => 2, 'value' => 5],
            ['from' => 1, 'to' => 2, 'value' => 3],
        ])],
    'MarimekkoChart' => [400, 300, fn() => (new FastChart\MarimekkoChart(400, 300))
        ->setColumns([
            ['label' => 'Q1', 'segments' => [
                ['label' => 'x', 'value' => 30],
                ['label' => 'y', 'value' => 20],
            ]],
            ['label' => 'Q2', 'segments' => [
                ['label' => 'x', 'value' => 40],
                ['label' => 'y', 'value' => 10],
            ]],
        ])],
    'VectorChart'  => [300, 300, fn() => (new FastChart\VectorChart(300, 300))
        ->setVectors([
            ['x' => 0, 'y' => 0, 'dx' => 1, 'dy' => 1],
            ['x' => 1, 'y' => 0, 'dx' => -1, 'dy' => 1],
            ['x' => 0, 'y' => 1, 'dx' => 1, 'dy' => -1],
        ])],
    'Code128'      => [300, 80, fn() => (new FastChart\Code128())
        ->setData('FC-12345')->setSize(300, 80)],
    'QrCode'       => [200, 200, fn() => (new FastChart\QrCode())
        ->setData('https://example.com')->setSize(200, 200)],
];

$fail = 0;
foreach ($families as $name => [$w, $h, $build]) {
    $c = $build();
    /* Charts pick up the system font via setFontPath; Symbol classes
     * use a fixed monospace bar layout and don't need a font. */
    if (method_exists($c, 'setFontPath')) {
        $c->setFontPath($font);
    }

    $jpg  = $c->renderJpeg();
    $webp = $c->renderWebp();

    $row_ok = true;

    $ij = @imagecreatefromstring($jpg);
    if (!$ij) {
        echo "FAIL $name JPEG: decode failed (", strlen($jpg), " bytes)\n";
        $row_ok = false;
    } else {
        if (imagesx($ij) !== $w || imagesy($ij) !== $h) {
            echo "FAIL $name JPEG dims: ", imagesx($ij), "x", imagesy($ij),
                 " != {$w}x{$h}\n";
            $row_ok = false;
        }
        /* No imagedestroy(): no-op since PHP 8.0, deprecated in 8.5. */
        $ij = null;
    }

    $iw = @imagecreatefromstring($webp);
    if (!$iw) {
        echo "FAIL $name WebP: decode failed (", strlen($webp), " bytes)\n";
        $row_ok = false;
    } else {
        if (imagesx($iw) !== $w || imagesy($iw) !== $h) {
            echo "FAIL $name WebP dims: ", imagesx($iw), "x", imagesy($iw),
                 " != {$w}x{$h}\n";
            $row_ok = false;
        }
        $iw = null;
    }

    if ($row_ok) {
        echo "$name: JPEG/WebP decode ok\n";
    } else {
        $fail++;
    }
}
echo $fail === 0 ? "ALL OK\n" : "FAILED $fail/" . count($families) . "\n";
?>
--EXPECT--
LineChart: JPEG/WebP decode ok
AreaChart: JPEG/WebP decode ok
BarChart: JPEG/WebP decode ok
PieChart: JPEG/WebP decode ok
ScatterChart: JPEG/WebP decode ok
StockChart: JPEG/WebP decode ok
RadarChart: JPEG/WebP decode ok
BubbleChart: JPEG/WebP decode ok
PolarChart: JPEG/WebP decode ok
SurfaceChart: JPEG/WebP decode ok
ContourChart: JPEG/WebP decode ok
GaugeChart: JPEG/WebP decode ok
GanttChart: JPEG/WebP decode ok
BoxPlot: JPEG/WebP decode ok
Treemap: JPEG/WebP decode ok
Funnel: JPEG/WebP decode ok
Waterfall: JPEG/WebP decode ok
Heatmap: JPEG/WebP decode ok
LinearMeter: JPEG/WebP decode ok
ArcDiagram: JPEG/WebP decode ok
ChordDiagram: JPEG/WebP decode ok
NetworkChart: JPEG/WebP decode ok
PopulationPyramid: JPEG/WebP decode ok
ViolinPlot: JPEG/WebP decode ok
CirclePacking: JPEG/WebP decode ok
Pictogram: JPEG/WebP decode ok
VennDiagram: JPEG/WebP decode ok
WordCloud: JPEG/WebP decode ok
SerpentineTimeline: JPEG/WebP decode ok
Dendrogram: JPEG/WebP decode ok
Partition: JPEG/WebP decode ok
BulletChart: JPEG/WebP decode ok
ParetoChart: JPEG/WebP decode ok
CalendarHeatmap: JPEG/WebP decode ok
SunburstChart: JPEG/WebP decode ok
SankeyChart: JPEG/WebP decode ok
MarimekkoChart: JPEG/WebP decode ok
VectorChart: JPEG/WebP decode ok
Code128: JPEG/WebP decode ok
QrCode: JPEG/WebP decode ok
ALL OK
