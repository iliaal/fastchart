--TEST--
StockChart: six indicator panes render without overflowing the layout array
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* Regression: the render function's local layout array was sized [3]
 * while FASTCHART_MAX_INDICATOR_PANES is 6, so 4-6 panes wrote out of
 * bounds on the stack (caught under ASAN). Render six panes and confirm
 * valid output; under the sanitizer build this trips the overflow if it
 * regresses. */

$rows = []; $ts = strtotime('2025-04-01'); $close = 100.0;
for ($i = 0; $i < 60; $i++) {
    $d = sin($i * 0.45) * 1.6 + 0.4;
    $open = $close; $close += $d;
    $rows[] = [$ts + $i * 86400, $open, max($open, $close) + 0.6, min($open, $close) - 0.6, $close, 1500];
}

$c = (new FastChart\StockChart(800, 760))
    ->setOhlcv($rows)
    ->addRSI(14)->addATR(14)->addCCI(20)
    ->addWilliamsR(14)->addStdDev(20)->addMomentum(10);

$svg = $c->renderSvg();
echo "valid: ", (strlen($svg) > 100 &&
    simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false) ? "yes" : "no", "\n";
echo "raster_ok: ", (strlen($c->renderPng()) > 100) ? "yes" : "no", "\n";

/* A seventh pane is rejected (cap is 6). */
try {
    $c->addRSI(7);
    echo "seventh_rejected: no\n";
} catch (\ValueError $e) {
    echo "seventh_rejected: yes\n";
}

echo "ok\n";
?>
--EXPECT--
valid: yes
raster_ok: yes
seventh_rejected: yes
ok
