--TEST--
StockChart::addMovingAverage supports SMA and EMA with configurable period
--EXTENSIONS--
fastchart
--FILE--
<?php

$rows = [];
$base = 1700000000;
for ($i = 0; $i < 30; $i++) {
    $p = 100 + sin($i / 4) * 5;
    $rows[] = [$base + $i * 86400, $p, $p + 0.5, $p - 0.5, $p + 0.2, 1000 + $i * 50];
}

/* Mix of SMA + EMA, plus the legacy bulk shortcut. The renderer uses
 * pal.series colors per overlay, so we just verify the chart renders
 * and the legend strings end up in the rendered bytes (text via FT
 * doesn't end up as literal bytes, but the chart should still produce
 * a non-trivial PNG). */
$im = imagecreatetruecolor(800, 400);
$out = (new FastChart\StockChart(800, 400))
    ->setOhlcv($rows)
    ->addMovingAverage(5, FastChart\StockChart::MA_SMA)
    ->addMovingAverage(10, FastChart\StockChart::MA_EMA)
    ->draw($im);
echo "mixed_sma_ema: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* setMovingAverages still works as bulk SMA shortcut. */
$im2 = imagecreatetruecolor(400, 200);
(new FastChart\StockChart(400, 200))
    ->setOhlcv($rows)
    ->setMovingAverages([3, 7])
    ->draw($im2);
echo "bulk_sma_shortcut: ok\n";

/* Validation: period < 2 */
try {
    (new FastChart\StockChart)->addMovingAverage(1);
    echo "period_low: no throw\n";
} catch (\ValueError $e) {
    echo "period_low: ValueError ok\n";
}

/* Validation: bogus type */
try {
    (new FastChart\StockChart)->addMovingAverage(10, 99);
    echo "bad_type: no throw\n";
} catch (\ValueError $e) {
    echo "bad_type: ValueError ok\n";
}

/* Cap at 8 overlays. */
$chart = new FastChart\StockChart;
for ($i = 0; $i < 8; $i++) $chart->addMovingAverage(2 + $i);
try {
    $chart->addMovingAverage(20);
    echo "ninth_overlay: no throw\n";
} catch (\ValueError $e) {
    echo "ninth_overlay: ValueError ok\n";
}

/* MA_SMA / MA_EMA constants are 0 and 1. */
echo "MA_SMA=", FastChart\StockChart::MA_SMA, " MA_EMA=", FastChart\StockChart::MA_EMA, "\n";
?>
--EXPECT--
mixed_sma_ema: ok
bulk_sma_shortcut: ok
period_low: ValueError ok
bad_type: ValueError ok
ninth_overlay: ValueError ok
MA_SMA=0 MA_EMA=1
