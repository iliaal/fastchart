--TEST--
Audit fixes: data:image URI, atomic setSeries, BoxPlot monotonic 5-number summary
--EXTENSIONS--
fastchart
--FILE--
<?php

/* CR-001: Chart::svgToPng/Jpeg/Webp must reject SVG that contains
 * a data:image/ URI. plutosvg's <image href="data:image/..."> loader
 * decodes the embedded raster directly, bypassing fastchart's
 * output-dim cap. A 10x10 root SVG carrying a 4097x4097 embedded
 * PNG would otherwise allocate ~67 MB inside plutosvg before our
 * cap check sees the actual rasterized dims. */
$bad = '<svg xmlns="http://www.w3.org/2000/svg" width="10" height="10">'
     . '<image href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUg==" width="10" height="10"/>'
     . '</svg>';
foreach (['svgToPng', 'svgToJpeg', 'svgToWebp'] as $m) {
    try {
        FastChart\Chart::$m($bad);
        echo "data_uri_accepted_$m: REGRESSION\n";
    } catch (\ValueError $e) {
        echo "data_uri_rejected_$m: ",
            (str_contains($e->getMessage(), 'data:image/') ? "ok" : "wrong-msg"), "\n";
    }
}

/* Case-insensitivity: same payload with uppercase "DATA:IMAGE/"
 * doesn't trigger plutosvg's loader (it does case-sensitive
 * matching), but rejecting it is harmless and defensive. */
$bad_upper = str_replace('data:image/', 'DATA:IMAGE/', $bad);
try { FastChart\Chart::svgToPng($bad_upper); echo "upper_accepted: REGRESSION\n"; }
catch (\ValueError $e) { echo "upper_rejected: ok\n"; }

/* CR-002: Line/Area/Bar setSeries() must be atomic on strict-mode
 * failure. The pre-fix code released self->series first, then
 * parsed; a TypeError thrown mid-parse left self with the accepted
 * prefix of the new data, and the original series was gone. */
foreach (['LineChart', 'AreaChart', 'BarChart'] as $cls) {
    $fqn = "FastChart\\$cls";
    $c = (new $fqn(400, 200))
        ->setStrict(true)
        ->setSeries([['data' => [1, 2, 3, 4, 5]]]);
    $before = $c->renderSvg();
    try {
        $c->setSeries([
            ['data' => [10, 20, 30]],
            ['data' => [1, 'oops', 3]],
        ]);
        echo "no_throw_$cls: REGRESSION\n";
    } catch (\TypeError $e) {
        $after = $c->renderSvg();
        echo "atomic_$cls: ", ($before === $after ? "ok" : "fail"), "\n";
    }
}

/* CR-003: BoxPlot::setBoxes() must reject unordered five-number
 * summaries. Pre-fix, min/q1/median/q3/max were parsed without
 * monotonicity check and could produce negative-height SVG rects. */
$c = (new FastChart\BoxPlot(400, 300))->setBoxes([
    ['label' => 'good', 'min' => 1, 'q1' => 2, 'median' => 3, 'q3' => 4, 'max' => 5],
    ['label' => 'bad',  'min' => 1, 'q1' => 5, 'median' => 3, 'q3' => 4, 'max' => 2],
]);
$svg_bad_dropped = $c->renderSvg();

/* Equivalent chart with only the good entry should render identically. */
$c2 = (new FastChart\BoxPlot(400, 300))->setBoxes([
    ['label' => 'good', 'min' => 1, 'q1' => 2, 'median' => 3, 'q3' => 4, 'max' => 5],
]);
$svg_good_only = $c2->renderSvg();
echo "boxplot_bad_dropped: ",
    ($svg_bad_dropped === $svg_good_only ? "ok" : "fail"), "\n";

/* No negative-height rect should ever escape. */
echo "boxplot_no_neg_rect: ",
    (preg_match('/height="-/', $svg_bad_dropped) ? "REGRESSION" : "ok"), "\n";

/* Positional shape is gated the same way. */
$c3 = (new FastChart\BoxPlot(400, 300))->setBoxes([
    [1, 2, 3, 4, 5],        /* good */
    [5, 4, 3, 2, 1],        /* bad: reversed */
]);
echo "boxplot_positional_validated: ",
    (preg_match('/height="-/', $c3->renderSvg()) ? "REGRESSION" : "ok"), "\n";

/* Bonus: Surface/ContourChart::setGrid() now uses parse-then-swap
 * (was clear-then-parse). A type-error on a bad input must leave
 * the original grid intact. */
foreach (['SurfaceChart', 'ContourChart'] as $cls) {
    $fqn = "FastChart\\$cls";
    $c = (new $fqn(400, 300))->setGrid([[1, 2, 3], [4, 5, 6]]);
    $before = $c->renderSvg();
    try { $c->setGrid('not an array'); } catch (\TypeError $e) {}
    $after = $c->renderSvg();
    echo "grid_atomic_$cls: ", ($before === $after ? "ok" : "fail"), "\n";
}

echo "ok\n";
?>
--EXPECT--
data_uri_rejected_svgToPng: ok
data_uri_rejected_svgToJpeg: ok
data_uri_rejected_svgToWebp: ok
upper_rejected: ok
atomic_LineChart: ok
atomic_AreaChart: ok
atomic_BarChart: ok
boxplot_bad_dropped: ok
boxplot_no_neg_rect: ok
boxplot_positional_validated: ok
grid_atomic_SurfaceChart: ok
grid_atomic_ContourChart: ok
ok
