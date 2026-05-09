--TEST--
StockChart: setOhlcv after addBollingerBands / addParabolicSAR clears stale overlays
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Regression for SS-001: setOhlcv replaced the candle buffer but
 * left overlay state intact. Bollinger / PSAR overlay arrays sized
 * parallel to the OLD candle count then iterated `ov->n` against
 * the NEW (possibly shorter) candle buffer at render time, reading
 * past the end of the heap allocation. Under ASan this aborted; in
 * production builds it produced garbage pixel coordinates from
 * uninitialised memory. */

function rows(int $n): array {
    $rows = [];
    $base = 1700000000;
    $price = 50.0;
    for ($i = 0; $i < $n; $i++) {
        $delta = sin($i / 3.0) * 1.5;
        $open  = $price;
        $close = $price + $delta;
        $high  = max($open, $close) + 0.4;
        $low   = min($open, $close) - 0.4;
        $rows[] = [$base + $i * 86400, $open, $high, $low, $close, 30000 + $i * 100];
        $price = $close;
    }
    return $rows;
}

$big   = rows(60);
$small = rows(20);

// 1) Trigger path: setOhlcv(big) → addBollingerBands → setOhlcv(small) → renderPng.
//    Pre-fix: overlay->n = 60, ov->a/b/c sized for 60, candles array now sized 20.
//    Render loop reads candles[i].ts for i up to 59 → OOB.
$chart = (new FastChart\StockChart(400, 240))
    ->setOhlcv($big)
    ->addBollingerBands(20, 2.0)
    ->setOhlcv($small);
$png = $chart->renderPng();
var_dump(strlen($png) > 0);
var_dump(substr($png, 0, 8) === "\x89PNG\r\n\x1a\n");

// 2) Same path with Parabolic SAR (the other price overlay kind).
$chart = (new FastChart\StockChart(400, 240))
    ->setOhlcv($big)
    ->addParabolicSAR(0.02, 0.2)
    ->setOhlcv($small);
$png = $chart->renderPng();
var_dump(strlen($png) > 0);

// 3) Both overlays at once, then shrink.
$chart = (new FastChart\StockChart(400, 240))
    ->setOhlcv($big)
    ->addBollingerBands(20, 2.0)
    ->addParabolicSAR(0.02, 0.2)
    ->setOhlcv($small);
$png = $chart->renderPng();
var_dump(strlen($png) > 0);

// 4) Re-adding the overlay after the second setOhlcv must work — the
//    fix clears overlays so the next add() starts from a clean slate.
$chart = (new FastChart\StockChart(400, 240))
    ->setOhlcv($big)
    ->addBollingerBands(20, 2.0)
    ->setOhlcv($small)
    ->addBollingerBands(10, 2.0);  // smaller period to fit shorter buffer
$png = $chart->renderPng();
var_dump(strlen($png) > 0);

// 5) setOhlcv with the SAME size as before (replacing not shrinking)
//    — still triggers the issue if overlays aren't cleared, because
//    the overlay's ov->a/b/c are now stale references to the freed
//    candles' indexed values. Without the fix, this is a use-after-
//    free of the OLD overlay arrays' indices into the NEW candles.
//    With the fix, overlays are cleared and the render is clean.
$chart = (new FastChart\StockChart(400, 240))
    ->setOhlcv($big)
    ->addBollingerBands(20, 2.0)
    ->setOhlcv($big);  // same length, different (recomputed) data
$png = $chart->renderPng();
var_dump(strlen($png) > 0);

?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
