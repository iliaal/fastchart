--TEST--
StockChart phase-2 indicators: MACD / Stochastic / BollingerBands / ParabolicSAR
--EXTENSIONS--
fastchart
gd
--FILE--
<?php
/* 80 OHLCV bars with a mid-period shock so each indicator has
 * something to react to. */
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
    $vol   = 1500 + abs($delta) * 220;
    $rows[] = [$ts + $i * 86400, $open, $high, $low, $close, $vol];
}

$baseline = (new FastChart\StockChart(560, 360))
    ->setOhlcv($rows)
    ->renderPng();

/* Each indicator must change the rendered bytes. */
$macd = (new FastChart\StockChart(560, 360))
    ->setOhlcv($rows)
    ->addMACD()
    ->renderPng();
echo "macd_renders:    ", ($macd !== $baseline ? "yes" : "no"), "\n";

$stoch = (new FastChart\StockChart(560, 360))
    ->setOhlcv($rows)
    ->addStochastic()
    ->renderPng();
echo "stoch_renders:   ", ($stoch !== $baseline ? "yes" : "no"), "\n";

$boll = (new FastChart\StockChart(560, 360))
    ->setOhlcv($rows)
    ->addBollingerBands()
    ->renderPng();
echo "boll_renders:    ", ($boll !== $baseline ? "yes" : "no"), "\n";

$psar = (new FastChart\StockChart(560, 360))
    ->setOhlcv($rows)
    ->addParabolicSAR()
    ->renderPng();
echo "psar_renders:    ", ($psar !== $baseline ? "yes" : "no"), "\n";

/* Different parameters must produce a different output. */
$macd_alt = (new FastChart\StockChart(560, 360))
    ->setOhlcv($rows)
    ->addMACD(8, 21, 5)
    ->renderPng();
echo "macd_params:     ", ($macd_alt !== $macd ? "differs" : "same"), "\n";

$boll_alt = (new FastChart\StockChart(560, 360))
    ->setOhlcv($rows)
    ->addBollingerBands(20, 1.5)
    ->renderPng();
echo "boll_params:     ", ($boll_alt !== $boll ? "differs" : "same"), "\n";

/* Order constraint: setOhlcv must come first. */
try {
    (new FastChart\StockChart(200, 120))->addMACD();
    echo "macd_no_ohlcv: no throw (unexpected)\n";
} catch (\Error $e) {
    echo "macd_no_ohlcv:   throws\n";
}

/* Parameter validation: fast >= slow must reject. */
try {
    (new FastChart\StockChart(560, 360))->setOhlcv($rows)->addMACD(26, 12);
    echo "macd_bad_periods: no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "macd_bad_periods: ValueError\n";
}

/* Bollinger negative stddev rejected. */
try {
    (new FastChart\StockChart(560, 360))->setOhlcv($rows)->addBollingerBands(20, -1.0);
    echo "boll_bad_stddev: no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "boll_bad_stddev: ValueError\n";
}

/* PSAR af_init > af_max rejected. */
try {
    (new FastChart\StockChart(560, 360))->setOhlcv($rows)->addParabolicSAR(0.3, 0.1);
    echo "psar_bad_af: no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "psar_bad_af:     ValueError\n";
}

/* Combined: MACD + Stochastic together (2 of 3 panes used) +
 * Bollinger + PSAR (price overlays). All must coexist. */
$combo = (new FastChart\StockChart(720, 520))
    ->setOhlcv($rows)
    ->addMACD()
    ->addStochastic()
    ->addBollingerBands()
    ->addParabolicSAR()
    ->renderPng();
echo "combo:           ", ($combo !== '' ? "ok" : "empty"), "\n";

/* Price-overlay cap: 4 overlays max (Bollinger + PSAR are 2;
 * 3 more PSARs would hit the cap, the 5th throws). */
try {
    (new FastChart\StockChart(720, 360))
        ->setOhlcv($rows)
        ->addParabolicSAR()
        ->addParabolicSAR()
        ->addParabolicSAR()
        ->addParabolicSAR()
        ->addParabolicSAR();   /* 5th must reject */
    echo "overlay_cap: no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "overlay_cap:     ValueError\n";
}
?>
--EXPECT--
macd_renders:    yes
stoch_renders:   yes
boll_renders:    yes
psar_renders:    yes
macd_params:     differs
boll_params:     differs
macd_no_ohlcv:   throws
macd_bad_periods: ValueError
boll_bad_stddev: ValueError
psar_bad_af:     ValueError
combo:           ok
overlay_cap:     ValueError
