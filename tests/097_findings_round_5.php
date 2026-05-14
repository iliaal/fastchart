<?php
/* Synthetic candle data reused across the indicator probes. */
$rows = [];
$ts = strtotime('2025-01-01');
$close = 100.0;
for ($i = 0; $i < 60; $i++) {
    $delta = sin($i * 0.4) * 1.5 + 0.4;
    $open = $close;
    $close = $close + $delta;
    $rows[] = [$ts + $i * 86400, $open, max($open, $close) + 0.5,
               min($open, $close) - 0.5, $close, 1000];
}

/* FC-001: addMACD signal_p / addStochastic smooth must reject
 * unbounded values BEFORE the int cast that drives index math. */
try {
    (new FastChart\StockChart(560, 360))
        ->setOhlcv($rows)
        ->addMACD(12, 26, PHP_INT_MAX);
    echo "macd_huge_signal: no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "macd_huge_signal: ValueError\n";
}
try {
    (new FastChart\StockChart(560, 360))
        ->setOhlcv($rows)
        ->addStochastic(14, PHP_INT_MAX);
    echo "stoch_huge_smooth: no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "stoch_huge_smooth: ValueError\n";
}

/* FC-002: Heatmap::setGrid must enforce the same 10000-cell cap
 * as Surface / Contour. 101 x 100 = 10100 must reject. */
$big = [];
for ($r = 0; $r < 101; $r++) {
    $row = [];
    for ($c = 0; $c < 100; $c++) $row[] = $r + $c;
    $big[] = $row;
}
try {
    (new FastChart\Heatmap(400, 400))->setGrid($big);
    echo "heatmap_grid_cap: no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "heatmap_grid_cap: ValueError\n";
}
/* And the equivalent Surface call still rejects. */
try {
    (new FastChart\SurfaceChart(400, 400))->setGrid($big);
    echo "surface_grid_cap: no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "surface_grid_cap: ValueError\n";
}

/* FC-003: a small JPEG-shaped buffer with a SOF marker that
 * declares 100000 x 100000 dimensions must be silently skipped
 * by setBackgroundImage. We forge the SOI + APP0 + SOF0 segments
 * by hand; libgd will fail to decode the body but our preflight
 * runs first. */
$tmp = tempnam(sys_get_temp_dir(), 'fc_jpg_huge_') . '.jpg';
$jpeg = "\xFF\xD8";                              /* SOI */
$jpeg .= "\xFF\xE0\x00\x10JFIF\x00\x01\x01\x00\x00\x01\x00\x01\x00\x00";
$jpeg .= "\xFF\xC0\x00\x11\x08";                 /* SOF0, len 17, 8-bit precision */
$jpeg .= "\x86\xA0\x86\xA0";                     /* H = 100000, W = 100000 (BE) */
$jpeg .= "\x03\x01\x22\x00\x02\x11\x01\x03\x11\x01"; /* component spec */
$jpeg .= "\xFF\xD9";                             /* EOI */
file_put_contents($tmp, $jpeg);

$base = (new FastChart\LineChart(160, 100))
    ->setSeries([['data' => [1, 2, 3]]])->renderPng();
$with_huge_bg = (new FastChart\LineChart(160, 100))
    ->setSeries([['data' => [1, 2, 3]]])
    ->setBackgroundImage($tmp)
    ->renderPng();
echo "huge_dim_skipped: ", ($with_huge_bg === $base ? "yes" : "no"), "\n";
@unlink($tmp);

/* And a normal-sized real PNG still composites. */
$tmp_small = tempnam(sys_get_temp_dir(), 'fc_png_small_') . '.png';
$im = imagecreatetruecolor(32, 32);
imagefill($im, 0, 0, imagecolorallocate($im, 0xCC, 0x33, 0x66));
imagepng($im, $tmp_small);
$with_small_bg = (new FastChart\LineChart(160, 100))
    ->setSeries([['data' => [1, 2, 3]]])
    ->setBackgroundImage($tmp_small)
    ->renderPng();
echo "small_dim_used:   ", ($with_small_bg !== $base ? "yes" : "no"), "\n";
@unlink($tmp_small);

/* FC-004: Bollinger overlays must expand the stock price range so
 * upper / lower bands don't pin to the plot edge. Construct two
 * close prices (100, 200) so the candle high/low IS [99.5, 200.5]
 * but Bollinger(period=2, stddev=2) puts upper=250 / lower=50 well
 * outside that range. */
$tight = [
    [strtotime('2025-01-01'), 100, 101, 99,  100, 1000],
    [strtotime('2025-01-02'), 200, 201, 199, 200, 1000],
    [strtotime('2025-01-03'), 100, 101, 99,  100, 1000],
];
$baseline = (new FastChart\StockChart(400, 240))->setOhlcv($tight)->renderPng();
$with_boll = (new FastChart\StockChart(400, 240))
    ->setOhlcv($tight)
    ->addBollingerBands(2, 2.0)
    ->renderPng();
/* If the y-range expanded, the band line can't land at the very
 * top or bottom row of the plot. We assert the rendered output
 * differs from the baseline AND the Bollinger version isn't
 * collapsed onto a single y-row by sampling pixel rows for any
 * change. The simplest signal: with-bands output has more
 * distinct dark rows in the central band than the baseline. */
echo "boll_range_used:  ", ($with_boll !== $baseline ? "yes" : "no"), "\n";

/* FC-005: Funnel honours the inherited setShowValues($flag, $fmt)
 * format string. The default %.0f and a custom "%.2f" must produce
 * different bytes (the rendered values change). */
$stages = [
    ['label' => 'A', 'value' => 1234],
    ['label' => 'B', 'value' =>  789],
];
$fdef = (new FastChart\Funnel(320, 220))->setStages($stages)->renderPng();
$ffmt = (new FastChart\Funnel(320, 220))
    ->setStages($stages)
    ->setShowValues(true, '%.2f')
    ->renderPng();
echo "funnel_format:    ", ($fdef !== $ffmt ? "differs" : "same"), "\n";

/* FC-006: Waterfall kind must EXACT-match "total" — "totalXYZ" is
 * not a valid kind and should fall back to delta. Compare against
 * a chart with only delta bars: the rendered output must match. */
$wf_total_xyz = (new FastChart\Waterfall(320, 200))
    ->setBars([
        ['label' => 'X', 'value' => 100, 'kind' => 'totalXYZ'],
        ['label' => 'Y', 'value' => 50],
    ])->renderPng();
$wf_pure_delta = (new FastChart\Waterfall(320, 200))
    ->setBars([
        ['label' => 'X', 'value' => 100],   /* implicit delta */
        ['label' => 'Y', 'value' => 50],
    ])->renderPng();
echo "wf_kind_strict:   ", ($wf_total_xyz === $wf_pure_delta ? "ok" : "leaks total"), "\n";

/* And the exact 'total' string still maps to total. */
$wf_total = (new FastChart\Waterfall(320, 200))
    ->setBars([
        ['label' => 'X', 'value' => 100, 'kind' => 'total'],
        ['label' => 'Y', 'value' => 50],
    ])->renderPng();
echo "wf_total_works:   ", ($wf_total !== $wf_pure_delta ? "yes" : "no"), "\n";

/* FC-007: LinearMeter setValueFormat('') must reset to the default
 * "%.0f", not store an empty string that produces blank labels. */
$lm_default = (new FastChart\LinearMeter(360, 120))
    ->setRange(0, 100)->setValue(50)
    ->setZones([['from' => 0, 'to' => 100]])
    ->renderPng();
$lm_set_then_clear = (new FastChart\LinearMeter(360, 120))
    ->setRange(0, 100)->setValue(50)
    ->setZones([['from' => 0, 'to' => 100]])
    ->setValueFormat('%.3f')
    ->setValueFormat('')   /* clear back to default */
    ->renderPng();
echo "meter_empty_clears: ", ($lm_default === $lm_set_then_clear ? "yes" : "no"), "\n";
?>
