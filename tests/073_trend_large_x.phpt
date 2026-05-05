--TEST--
Polynomial trend stays numerically stable on large-x inputs (timestamps)
--EXTENSIONS--
fastchart
--FILE--
<?php

// Timestamps near 1.7e9 with x^6 = 2.4e55 used to overflow / lose
// all precision in the Vandermonde matrix. After x-normalization
// the matrix entries are O(1).
$points = [];
$base = 1700000000;
for ($i = 0; $i < 50; $i++) {
    $x = $base + $i * 86400;          // daily samples
    $y = 100 + 0.5 * $i + 0.01 * $i * $i; // quadratic + a bit of noise
    $points[] = [$x, $y];
}

foreach ([1, 2, 3] as $deg) {
    $bytes = (new FastChart\ScatterChart(500, 400))
        ->setPoints($points)
        ->setTrendLine(true, 0xFF0000, $deg)
        ->renderPng();
    echo "deg_{$deg}_renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";
}

// Degree cap is 3 now; 4 must throw.
try {
    (new FastChart\ScatterChart)->setTrendLine(true, null, 4);
    echo "deg_4_cap: no throw\n";
} catch (\ValueError $e) {
    echo "deg_4_cap: ValueError ok\n";
}
?>
--EXPECT--
deg_1_renders: ok
deg_2_renders: ok
deg_3_renders: ok
deg_4_cap: ValueError ok
