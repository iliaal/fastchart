--TEST--
StockChart::addRSI / addMomentum / addROC / addOBV emit panes with the right shape
--EXTENSIONS--
fastchart
gd
--FILE--
<?php
/* Synthetic OHLCV: 60 bars of a smooth uptrend with occasional dips,
 * volume that varies. Enough samples that 14-period indicators have
 * a stable warm-up window. */
$rows = [];
$ts = strtotime('2025-01-01');
$close = 100.0;
for ($i = 0; $i < 60; $i++) {
    $delta = sin($i * 0.4) * 2.0 + 0.3;        /* mostly up */
    $open  = $close;
    $close = $close + $delta;
    $high  = max($open, $close) + 0.5;
    $low   = min($open, $close) - 0.5;
    $vol   = 1000 + ($i % 7) * 200;
    $rows[] = [$ts + $i * 86400, $open, $high, $low, $close, $vol];
}

/* Each indicator should produce a render that differs from a
 * baseline render with no indicators — proves the pane lands and
 * draws. Only one indicator at a time so we don't blow the
 * 3-pane cap when stacking. */
function render_with(array $rows, ?string $kind, $arg = null): string {
    $c = (new FastChart\StockChart(560, 360))->setOhlcv($rows);
    if ($kind === 'rsi') $c->addRSI($arg ?? 14);
    elseif ($kind === 'mom') $c->addMomentum($arg ?? 10);
    elseif ($kind === 'roc') $c->addROC($arg ?? 10);
    elseif ($kind === 'obv') $c->addOBV();
    return $c->renderPng();
}

$base = render_with($rows, null);
echo "rsi_renders: ", (render_with($rows, 'rsi') !== $base ? "yes" : "no"), "\n";
echo "mom_renders: ", (render_with($rows, 'mom') !== $base ? "yes" : "no"), "\n";
echo "roc_renders: ", (render_with($rows, 'roc') !== $base ? "yes" : "no"), "\n";
echo "obv_renders: ", (render_with($rows, 'obv') !== $base ? "yes" : "no"), "\n";

/* Order constraint: setOhlcv must come first. */
try {
    (new FastChart\StockChart(200, 120))->addRSI(14);
    echo "rsi_no_ohlcv: no throw (unexpected)\n";
} catch (\Error $e) {
    echo "rsi_no_ohlcv: throws\n";
}

/* Period must be < candle count. */
try {
    (new FastChart\StockChart(200, 120))->setOhlcv($rows)->addRSI(60);
    echo "rsi_period_eq_n: no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "rsi_period_eq_n: ValueError\n";
}

/* Pane cap: 3 panes max. addOBV after two existing panes still
 * fits; a fourth must reject. */
try {
    (new FastChart\StockChart(560, 360))
        ->setOhlcv($rows)
        ->addRSI(14)
        ->addMomentum(10)
        ->addROC(10)
        ->addOBV();
    echo "fourth_pane: no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "fourth_pane: ValueError\n";
}
?>
--EXPECT--
rsi_renders: yes
mom_renders: yes
roc_renders: yes
obv_renders: yes
rsi_no_ohlcv: throws
rsi_period_eq_n: ValueError
fourth_pane: ValueError
