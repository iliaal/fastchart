<?php
/* Phase-2 stock indicators: MACD / Stochastic in their own panes,
 * Bollinger Bands + Parabolic SAR overlaid on the price pane.
 *
 *   - addMACD($fast = 12, $slow = 26, $signal = 9)
 *       MACD line + signal line + signed-by-direction histogram.
 *   - addStochastic($period = 14, $smooth = 3)
 *       %K and %D lines, range pinned to [0, 100].
 *   - addBollingerBands($period = 20, $stddev = 2.0)
 *       middle SMA + upper / lower bands at ±n·σ. Price overlay.
 *   - addParabolicSAR($af_init = 0.02, $af_max = 0.2)
 *       Wilder's trend-following stop. Dot-per-bar overlay.
 *
 * setOhlcv() must precede the addX() call — values are computed
 * eagerly and don't recompute on a subsequent setOhlcv. */

require __DIR__ . '/_bootstrap.php';

/* Synthetic 80-day OHLCV with a mid-period shock so MACD/Stoch
 * actually have a regime change to react to. */
$rows = [];
$ts = strtotime('2025-04-01');
$close = 100.0;
for ($i = 0; $i < 80; $i++) {
    $shock = $i === 35 ? -8.0 : ($i === 36 ? -4.0 : 0.0);
    $delta = sin($i * 0.45) * 1.6 + 0.3 + $shock;
    $open  = $close;
    $close = $close + $delta;
    $high  = max($open, $close) + 0.6;
    $low   = min($open, $close) - 0.6;
    $vol   = 1500 + abs($delta) * 220 + ($i % 5) * 80;
    $rows[] = [$ts + $i * 86400, $open, $high, $low, $close, $vol];
}

/* MACD with the conventional (12, 26, 9). */
(new FastChart\StockChart(720, 480))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('MACD(12, 26, 9)')
    ->setOhlcv($rows)
    ->addMACD()
    ->renderToFile(__DIR__ . '/40a_stock_macd.png');

/* Stochastic %K / %D with 14-period and 3-period smoothing. */
(new FastChart\StockChart(720, 480))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Stochastic(14, 3)')
    ->setOhlcv($rows)
    ->addStochastic()
    ->renderToFile(__DIR__ . '/40b_stock_stochastic.png');

/* Bollinger Bands overlaid on price. */
(new FastChart\StockChart(720, 460))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Bollinger Bands (20, 2σ)')
    ->setOhlcv($rows)
    ->addBollingerBands()
    ->renderToFile(__DIR__ . '/40c_stock_bollinger.png');

/* Parabolic SAR dots over price. */
(new FastChart\StockChart(720, 460))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Parabolic SAR (0.02, 0.2)')
    ->setOhlcv($rows)
    ->addParabolicSAR()
    ->renderToFile(__DIR__ . '/40d_stock_psar.png');

/* Combined chart: Bollinger + PSAR overlays + MACD + Stoch panes
 * — typical multi-indicator workflow. */
(new FastChart\StockChart(720, 600))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Combined: Bollinger + PSAR + MACD + Stoch')
    ->setOhlcv($rows)
    ->addBollingerBands()
    ->addParabolicSAR()
    ->addMACD()
    ->addStochastic()
    ->renderToFile(__DIR__ . '/40e_stock_combined.png');
