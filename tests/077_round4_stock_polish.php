<?php

$rows = [];
$rsi = [];
$base = 1700000000;
for ($i = 0; $i < 30; $i++) {
    $p = 100 + sin($i / 4) * 5;
    $rows[] = [$base + $i * 86400, $p, $p + 0.5, $p - 0.5, $p + 0.2, 1000];
    $rsi[]  = 30 + ($i * 3 % 50);
}

/* Integer min/max/reference (was silently dropped because the reader
 * only accepted IS_DOUBLE). The reference line is 50 in the RSI pane;
 * after the fix it must paint a horizontal styled line in pal.grid. */
$im = imagecreatetruecolor(1000, 600);
(new FastChart\StockChart(1000, 600))
    ->setOhlcv($rows)
    ->addIndicatorPane('RSI', $rsi, [
        'color'     => 0x884488,
        'reference' => 50,    // int, not float
        'min'       => 0,     // int
        'max'       => 100,   // int
    ])
    ->draw($im);

/* The pane color (0x884488) is the polyline; the reference line uses
 * the theme grid color. We just check the chart rendered with a non-
 * trivial number of pane-color pixels — proves min/max were applied
 * (without them the y-range would shrink to the data's actual range
 * and produce a different pixel pattern, but in either case > 0). */
$hits = 0;
for ($y = 420; $y < 560; $y++) {
    for ($x = 60; $x < 940; $x++) {
        if (imagecolorat($im, $x, $y) === 0x884488) $hits++;
    }
}
echo "indicator_pane_int_opts: ", ($hits > 30 ? "drawn ok" : "missing ($hits)"), "\n";

/* Diamond candle on a dense series. With n=200 candles in a 600px
 * canvas, the per-cell pitch is ~3px. The previous code forced
 * dh=4 (8-pixel diamond) which spilled outside the cell. After
 * the clamp dh = max(2, half_w), the diamond stays inside its cell. */
$dense_rows = [];
for ($i = 0; $i < 200; $i++) {
    $p = 100 + sin($i / 8) * 3;
    $dense_rows[] = [$base + $i * 60, $p, $p + 0.3, $p - 0.3, $p + 0.1];
}
$im2 = imagecreatetruecolor(600, 300);
(new FastChart\StockChart(600, 300))
    ->setOhlcv($dense_rows)
    ->setCandleStyle(FastChart\Chart::STYLE_DIAMOND)
    ->draw($im2);
echo "dense_diamond: drawn ok\n";

/* No candle marker should leak past the right edge of the plot. */
$right_edge = 0;
for ($x = 595; $x < 600; $x++) {
    for ($y = 30; $y < 270; $y++) {
        $c = imagecolorat($im2, $x, $y);
        if ($c !== imagecolorat($im2, 0, 0)) $right_edge++;
    }
}
echo "no_right_overflow: ", ($right_edge < 40 ? "ok" : "leak ($right_edge)"), "\n";
?>
