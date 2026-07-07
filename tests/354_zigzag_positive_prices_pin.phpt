--TEST--
StockChart::addZigZag pivot geometry is unchanged for positive prices
--EXTENSIONS--
fastchart
--FILE--
<?php

/* addZigZag now bases its reversal threshold on fabs(ext) with a small
 * absolute floor so series whose running extreme is <= 0 (spreads, returns)
 * still produce reversals. For strictly positive prices fabs(ext) == ext, so
 * this fixture must yield the exact same pivot segments as before the change.
 * This is a no-regression pin: it passes on both the pre- and post-fix build. */

use FastChart\StockChart;

$prices = [100, 110, 120, 108, 96, 105, 118, 130, 115, 100];
$rows = [];
foreach ($prices as $i => $p) {
    $rows[] = [1700000000 + $i * 86400, $p, $p + 1, $p - 1, $p, 1000];
}

$svg = (new StockChart(900, 500))
    ->setOhlcv($rows)
    ->addZigZag(5.0)
    ->renderSvg();

// Each ZigZag leg is a stroked overlay line; count the segments (pivots - 1).
$segments = preg_match_all('/stroke="#FF7F0E" stroke-width="2"/', $svg);
echo "zigzag_segments: ", $segments, "\n";

?>
--EXPECT--
zigzag_segments: 4
