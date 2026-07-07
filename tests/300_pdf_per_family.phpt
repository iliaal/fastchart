--TEST--
PDF: every concrete chart family emits a structurally valid vector PDF
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die("skip no system font available\n");
try {
    (new FastChart\LineChart(80, 60))->setSeries([1, 2, 3])->renderPdf();
} catch (\Error $e) {
    if (strpos($e->getMessage(), "PDF support not compiled in") !== false) {
        die("skip extension built without --with-pdfio\n");
    }
    throw $e;
}
?>
--INI--
asan.detect_leaks=0
--FILE--
<?php
/* Before this test, renderPdf() had coverage on only 4 chart families
 * (Line/Bar/Pie plus a rotated-label case in tests/200). A NaN/Inf
 * coordinate leaking into the content stream, or a family whose PDF
 * path never emitted a valid trailer, would go unseen. Build a minimal
 * instance of every concrete chart family and assert each renderPdf()
 * produces a %PDF- header, a startxref trailer keyword, and no literal
 * "nan"/"inf" tokens (a formatted non-finite coordinate). */

require __DIR__ . '/_font_candidates.inc';
$font = fc_pick_font();

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
    'BulletChart'  => fn() => (new FastChart\BulletChart(400, 80))
        ->setRange(0, 100)
        ->setBands([['from' => 0, 'to' => 60], ['from' => 60, 'to' => 85],
                    ['from' => 85, 'to' => 100]])
        ->setValue(72)->setTarget(80),
    'ParetoChart'  => fn() => (new FastChart\ParetoChart(400, 300))
        ->setBars([['label' => 'a', 'value' => 40],
                   ['label' => 'b', 'value' => 30],
                   ['label' => 'c', 'value' => 10]]),
    'CalendarHeatmap' => fn() => (new FastChart\CalendarHeatmap(600, 140))
        ->setData(['2026-01-05' => 3, '2026-02-14' => 9, '2026-03-15' => 5])
        ->setColorRamp(0xEEFFEE, 0x004400),
    'SunburstChart' => fn() => (new FastChart\SunburstChart(300, 300))
        ->setHierarchy(['label' => 'root', 'children' => [
            ['label' => 'A', 'value' => 10], ['label' => 'B', 'value' => 20]]]),
    'SankeyChart'  => fn() => (new FastChart\SankeyChart(400, 250))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 2, 'value' => 5],
                    ['from' => 1, 'to' => 2, 'value' => 3]]),
    'MarimekkoChart' => fn() => (new FastChart\MarimekkoChart(400, 300))
        ->setColumns([
            ['label' => 'Q1', 'segments' => [
                ['label' => 'x', 'value' => 30], ['label' => 'y', 'value' => 20]]],
            ['label' => 'Q2', 'segments' => [
                ['label' => 'x', 'value' => 40], ['label' => 'y', 'value' => 10]]],
        ]),
    'VectorChart'  => fn() => (new FastChart\VectorChart(300, 300))
        ->setVectors([['x' => 0, 'y' => 0, 'dx' => 1, 'dy' => 1],
                      ['x' => 1, 'y' => 0, 'dx' => -1, 'dy' => 1],
                      ['x' => 0, 'y' => 1, 'dx' => 1, 'dy' => -1]]),
    'ArcDiagram'   => fn() => (new FastChart\ArcDiagram(400, 200))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 1],
                    ['from' => 1, 'to' => 2, 'value' => 2]]),
    'ChordDiagram' => fn() => (new FastChart\ChordDiagram(300, 300))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 1],
                    ['from' => 1, 'to' => 2, 'value' => 2]]),
    'NetworkChart' => fn() => (new FastChart\NetworkChart(300, 300))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 1],
                    ['from' => 1, 'to' => 2, 'value' => 2]]),
    'PopulationPyramid' => fn() => (new FastChart\PopulationPyramid(300, 300))
        ->setCategories(['a', 'b', 'c'])
        ->setLeftSeries(['label' => 'L', 'data' => [1, 2, 3]])
        ->setRightSeries(['label' => 'R', 'data' => [2, 3, 1]]),
    'ViolinPlot'   => fn() => (new FastChart\ViolinPlot(300, 300))
        ->setGroups([['label' => 'X', 'values' => [1, 2, 3, 4, 3, 2]]]),
    'CirclePacking' => fn() => (new FastChart\CirclePacking(300, 300))
        ->setHierarchy(['children' => [['value' => 5], ['value' => 3], ['value' => 8]]]),
    'Pictogram'    => fn() => (new FastChart\Pictogram(300, 150))
        ->setTotal(10)->setValue(6),
    'VennDiagram'  => fn() => (new FastChart\VennDiagram(300, 300))
        ->setSets([['size' => 10], ['size' => 8]])
        ->setIntersections([['sets' => [0, 1], 'size' => 3]]),
    'WordCloud'    => fn() => (new FastChart\WordCloud(300, 300))
        ->setWords([['text' => 'alpha', 'weight' => 5],
                    ['text' => 'beta', 'weight' => 3],
                    ['text' => 'gamma', 'weight' => 8]]),
    'SerpentineTimeline' => fn() => (new FastChart\SerpentineTimeline(400, 200))
        ->setEvents([['label' => 'a', 'date' => 'Jan'],
                     ['label' => 'b', 'date' => 'Feb'],
                     ['label' => 'c', 'date' => 'Mar']]),
    'Dendrogram'   => fn() => (new FastChart\Dendrogram(300, 300))
        ->setHierarchy(['children' => [['value' => 5], ['value' => 3], ['value' => 8]]]),
    'Partition'    => fn() => (new FastChart\Partition(300, 300))
        ->setHierarchy(['children' => [['value' => 5], ['value' => 3], ['value' => 8]]]),
];

$fail = 0;
foreach ($families as $name => $build) {
    $c = $build();
    if (method_exists($c, 'setFontPath')) {
        $c->setFontPath($font);
    }
    $pdf = $c->renderPdf();

    $ok = true;
    if (!str_starts_with($pdf, "%PDF-")) {
        echo "FAIL $name: no %PDF- header\n"; $ok = false;
    }
    if (!str_contains($pdf, "startxref")) {
        echo "FAIL $name: no startxref trailer\n"; $ok = false;
    }
    if (strlen($pdf) < 400) {
        echo "FAIL $name: implausibly small (", strlen($pdf), " bytes)\n"; $ok = false;
    }
    /* A NaN/Inf coordinate surfaces as a literal "nan"/"inf" number
     * token. Content streams are FlateDecode-compressed (unreadable
     * here, and their random bytes would false-match), so strip them
     * and scan the uncompressed object/xref/trailer structure. Word
     * boundaries avoid matching the legitimate "/Info" trailer key. */
    $scan = preg_replace('/stream.*?endstream/s', '', $pdf);
    if (preg_match('/\bnan\b/i', $scan)) {
        echo "FAIL $name: contains 'nan' token\n"; $ok = false;
    }
    if (preg_match('/\binf\b/i', $scan)) {
        echo "FAIL $name: contains 'inf' token\n"; $ok = false;
    }
    if ($ok) echo "$name: ok\n";
    else $fail++;
}
echo $fail === 0 ? "ALL OK\n" : "FAILED $fail/" . count($families) . "\n";
?>
--EXPECT--
LineChart: ok
AreaChart: ok
BarChart: ok
PieChart: ok
ScatterChart: ok
StockChart: ok
RadarChart: ok
BubbleChart: ok
PolarChart: ok
SurfaceChart: ok
ContourChart: ok
GaugeChart: ok
GanttChart: ok
BoxPlot: ok
Treemap: ok
Funnel: ok
Waterfall: ok
Heatmap: ok
LinearMeter: ok
BulletChart: ok
ParetoChart: ok
CalendarHeatmap: ok
SunburstChart: ok
SankeyChart: ok
MarimekkoChart: ok
VectorChart: ok
ArcDiagram: ok
ChordDiagram: ok
NetworkChart: ok
PopulationPyramid: ok
ViolinPlot: ok
CirclePacking: ok
Pictogram: ok
VennDiagram: ok
WordCloud: ok
SerpentineTimeline: ok
Dendrogram: ok
Partition: ok
ALL OK
