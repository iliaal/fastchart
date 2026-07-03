--TEST--
StockChart folds generic overlay series into the price y-range
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression (fnd_ea621070): a generic overlay added with the base
 * addOverlaySeries() (stored in config["overlays"]) is drawn against the
 * candle price range but was excluded from the range computation, so an
 * overlay value outside the candle high/low clamped flat against the plot
 * edge. The range must expand to include finite overlay values. */

$ohlcv = [[1, 10, 12, 9, 11, 0], [2, 11, 13, 10, 12, 0]]; // prices ~9..13

$with = (new FastChart\StockChart(400, 300))
    ->setOhlcv($ohlcv)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->addOverlaySeries('line', [50, 60]) // far above candle range
    ->renderSvg();

$without = (new FastChart\StockChart(400, 300))
    ->setOhlcv($ohlcv)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->renderSvg();

// With the overlay folded in, the axis must reach up to ~60; without it,
// the axis tops out near the candle high (~13).
$reaches_60 = (strpos($with, '>60<') !== false || strpos($with, '>55<') !== false
               || strpos($with, '>50<') !== false);
echo "overlay expands range: ", $reaches_60 ? "yes" : "no", "\n";
echo "baseline stays low: ", (strpos($without, '>50<') === false
                              && strpos($without, '>60<') === false) ? "yes" : "no", "\n";

?>
--EXPECT--
overlay expands range: yes
baseline stays low: yes
