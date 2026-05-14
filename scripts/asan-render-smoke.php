<?php
/*
 * Render every chart family across PNG/JPEG/WebP/SVG and exit clean.
 * Used by the ASAN CI job's "Render-only leak smoke" step to confirm
 * fastchart itself reports zero LSan leaks without ext/gd in the
 * picture. Any leak the run surfaces is fastchart's.
 *
 * Mirrors tests/131_raster_formats_per_family.phpt's setter surface
 * but skips the ext/gd PHP-side decode — this script's job is leak
 * coverage, not output validation.
 */

$lato = '/usr/share/fonts/truetype/lato/Lato-Regular.ttf';
$font = is_readable($lato) ? $lato
        : '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf';

$ohlcv = [];
for ($i = 0; $i < 8; $i++) {
    $ohlcv[] = [1700000000 + $i * 86400,
                100 + $i, 102 + $i, 99 + $i, 101 + $i, 1000];
}

$families = [
    fn() => (new FastChart\LineChart(300, 200))->setSeries([1, 2, 3, 4, 5]),
    fn() => (new FastChart\AreaChart(300, 200))->setSeries([1, 2, 3, 4, 5]),
    fn() => (new FastChart\BarChart(300, 200))->setSeries([1, 5, 3]),
    fn() => (new FastChart\PieChart(300, 200))->setSlices(['a' => 1, 'b' => 2, 'c' => 3]),
    fn() => (new FastChart\ScatterChart(300, 200))->setPoints([[1, 1], [2, 3], [3, 2]]),
    fn() => (new FastChart\StockChart(400, 250))->setOhlcv($ohlcv),
    fn() => (new FastChart\RadarChart(300, 300))
            ->setSeries([['data' => [3, 4, 5, 4, 3]]])
            ->setCategoryLabels(['a', 'b', 'c', 'd', 'e']),
    fn() => (new FastChart\BubbleChart(300, 200))->setPoints([[1, 1, 10], [2, 3, 20]]),
    fn() => (new FastChart\PolarChart(300, 300))
            ->setSeries([['data' => [[0, 1], [45, 2], [90, 3]]]]),
    fn() => (new FastChart\SurfaceChart(300, 200))->setGrid([[1, 2, 3], [4, 5, 6], [7, 8, 9]]),
    fn() => (new FastChart\ContourChart(300, 200))->setGrid([[1, 2, 3], [4, 5, 6], [7, 8, 9]]),
    fn() => (new FastChart\GaugeChart(300, 200))->setValue(42),
    fn() => (new FastChart\GanttChart(400, 200))->setTasks([
        ['label' => 't1', 'start' => 0, 'end' => 5],
        ['label' => 't2', 'start' => 3, 'end' => 8],
    ]),
    fn() => (new FastChart\BoxPlot(300, 200))->setBoxes([
        ['min' => 1, 'q1' => 2, 'median' => 3, 'q3' => 4, 'max' => 5],
    ]),
    fn() => (new FastChart\Treemap(300, 200))->setItems([
        ['label' => 'a', 'value' => 5], ['label' => 'b', 'value' => 3],
    ]),
    fn() => (new FastChart\Funnel(300, 200))->setStages([
        ['label' => 'a', 'value' => 100], ['label' => 'b', 'value' => 50],
    ]),
    fn() => (new FastChart\Waterfall(300, 200))->setBars([
        ['label' => 'a', 'value' => 100], ['label' => 'b', 'value' => -20],
    ]),
    fn() => (new FastChart\Heatmap(300, 200))->setGrid([[1, 2], [3, 4]]),
    fn() => (new FastChart\LinearMeter(300, 60))->setValue(40),
    fn() => (new FastChart\Code128())->setData('FC-12345')->setSize(300, 80),
    fn() => (new FastChart\QrCode())->setData('https://example.com')->setSize(200, 200),
];

foreach ($families as $build) {
    $c = $build();
    if (method_exists($c, 'setFontPath') && is_readable($font)) {
        $c->setFontPath($font);
    }
    $c->renderPng();
    $c->renderJpeg();
    $c->renderWebp();
    $c->renderSvg();
}

echo "ok\n";
