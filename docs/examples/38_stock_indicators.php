<?php
/* Native stock indicators: each method computes from the typed
 * candle array set by setOhlcv() and pushes a regular indicator
 * pane.
 *
 *   - addRSI($period = 14)        : Wilder's relative strength index
 *   - addMomentum($period = 10)   : close[i] - close[i-period]
 *   - addROC($period = 10)        : pct change vs `period` bars ago
 *   - addOBV()                    : cumulative signed volume
 *
 * Up to 3 panes per chart; combine as needed. setOhlcv() must
 * precede the addX() call; the values are computed at add time. */

require __DIR__ . '/_bootstrap.php';

/* Synthetic 60-day OHLCV series: gentle uptrend with a mid-period
 * pullback so the indicators have something to react to. */
$rows = [];
$ts = strtotime('2025-04-01');
$close = 100.0;
for ($i = 0; $i < 60; $i++) {
    $shock = $i === 25 ? -8.0 : ($i === 26 ? -5.0 : 0.0);
    $delta = sin($i * 0.45) * 1.6 + 0.4 + $shock;
    $open  = $close;
    $close = $close + $delta;
    $high  = max($open, $close) + 0.6;
    $low   = min($open, $close) - 0.6;
    $vol   = 1500 + abs($delta) * 220 + ($i % 5) * 80;
    $rows[] = [$ts + $i * 86400, $open, $high, $low, $close, $vol];
}

/* RSI + a 20-day SMA above. */
(new FastChart\StockChart(720, 480))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Equity with RSI(14)')
    ->setOhlcv($rows)
    ->addMovingAverage(20, FastChart\StockChart::MA_SMA)
    ->addRSI(14)
    ->renderToFile(__DIR__ . '/38a_stock_rsi.png');

/* Momentum + ROC stacked. */
(new FastChart\StockChart(720, 520))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Equity with Momentum + ROC')
    ->setOhlcv($rows)
    ->addMomentum(10)
    ->addROC(10)
    ->renderToFile(__DIR__ . '/38b_stock_momentum_roc.png');

/* OBV: cumulative; works without a period. */
(new FastChart\StockChart(720, 480))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Equity with OBV')
    ->setOhlcv($rows)
    ->addOBV()
    ->renderToFile(__DIR__ . '/38c_stock_obv.png');
