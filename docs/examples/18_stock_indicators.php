<?php
/* Stock with indicator panes and per-bar volume colors:
 *   - addIndicatorPane stacks a custom data strip below the price
 *   - setVolumeColors paints volume bars in arbitrary RGB instead
 *     of the candle-direction default
 *   - setMovingAverages is the bulk shortcut for several SMAs at
 *     once (equivalent to repeated addMovingAverage(p, MA_SMA))
 *   - setDateAxisStride controls the X-axis tick density
 */

require __DIR__ . '/_bootstrap.php';

mt_srand(7);
$rows = [];
$base = 1700000000;
$price = 50.0;
$rsi = [];
for ($i = 0; $i < 60; $i++) {
    $delta = (mt_rand(-100, 100) / 100.0) * 1.4;
    $open  = $price;
    $close = $price + $delta;
    $high  = max($open, $close) + mt_rand(0, 60) / 100.0;
    $low   = min($open, $close) - mt_rand(0, 60) / 100.0;
    $vol   = 30000 + mt_rand(0, 25000);
    $rows[] = [$base + $i * 86400, $open, $high, $low, $close, $vol];
    $rsi[]  = 30 + 40 * (sin($i / 6) * 0.5 + 0.5);
    $price  = $close;
}

(new FastChart\StockChart(820, 520))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('ACME with RSI indicator pane')
    ->setOhlcv($rows)
    ->setMovingAverages([10, 20, 50])  // bulk shortcut, all SMAs
    ->setVolumePane(true)
    ->setVolumeColors(array_fill(0, 60, 0x9D9D9D))  // neutral gray volume
    ->addIndicatorPane('RSI(14)', $rsi, ['color' => 0x9B5DE5])
    ->setDateAxisStride(FastChart\Chart::DATE_WEEK, 2)
    ->renderToFile(__DIR__ . '/18_stock_indicators.png');
