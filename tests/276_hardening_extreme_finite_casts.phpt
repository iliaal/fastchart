--TEST--
Extreme finite inputs no longer reach undefined float-to-int casts
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: heatmap/surface normalization, scatter/vector pixel
 * mapping, and the stock volume bar each cast a float to int after a
 * divide whose operands could overflow to +/-Inf, making the quotient
 * NaN/Inf. (int)NaN is UB (C11 6.3.1.4p1). Finite-but-extreme data
 * must render finite geometry, not crash the process. */

$M = 1.7e308; /* near DBL_MAX */

function finite(string $png): bool { return strlen($png) > 50; }

$hm = (new FastChart\Heatmap(200, 200))
    ->setGrid([[-$M, $M], [$M, -$M]])->renderPng();
echo "heatmap: ", finite($hm) ? "ok" : "BAD", "\n";

$sf = (new FastChart\SurfaceChart(200, 200))
    ->setGrid([[-$M, $M], [$M, -$M]])->renderPng();
echo "surface: ", finite($sf) ? "ok" : "BAD", "\n";

$sc = (new FastChart\ScatterChart(300, 200))
    ->setPoints([[-$M, 1], [$M, 2]])->setTrendLine(true)->renderPng();
echo "scatter: ", finite($sc) ? "ok" : "BAD", "\n";

$vc = (new FastChart\VectorChart(300, 300))
    ->setVectors([['x' => -$M, 'y' => $M, 'dx' => 1, 'dy' => 1]])->renderPng();
echo "vector: ", finite($vc) ? "ok" : "BAD", "\n";

$rows = [[1700000000, 100.0, 101.0, 99.0, 100.5, $M]];
$stk = (new FastChart\StockChart(400, 250))
    ->setOhlcv($rows)->renderPng();
echo "stock_volume: ", finite($stk) ? "ok" : "BAD", "\n";

?>
--EXPECT--
heatmap: ok
surface: ok
scatter: ok
vector: ok
stock_volume: ok
