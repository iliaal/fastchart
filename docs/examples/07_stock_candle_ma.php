<?php
/* Stock chart with OHLCV candles, three moving-average overlays
 * (SMA / EMA / WMA), and a volume pane below. */

require __DIR__ . '/_bootstrap.php';

mt_srand(1);
$rows = [];
$base = 1700000000;
$price = 100.0;
for ($i = 0; $i < 90; $i++) {
    $open  = $price;
    $close = $price + (mt_rand(-100, 100) / 100.0) * 1.8;
    $high  = max($open, $close) + mt_rand(0, 80) / 100.0;
    $low   = min($open, $close) - mt_rand(0, 80) / 100.0;
    $vol   = 50000 + mt_rand(0, 30000);
    $rows[] = [$base + $i * 86400, $open, $high, $low, $close, $vol];
    $price = $close;
}

(new FastChart\StockChart(800, 460))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('ACME — 90 day OHLCV')
    ->setOhlcv($rows)
    ->addMovingAverage(10, FastChart\StockChart::MA_SMA)
    ->addMovingAverage(20, FastChart\StockChart::MA_EMA)
    ->addMovingAverage(30, FastChart\StockChart::MA_WMA)
    ->setVolumePane(true)
    ->setCandleStyle(FastChart\Chart::STYLE_CANDLE)
    ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT)
    ->renderToFile(__DIR__ . '/07_stock_candle_ma.png');
