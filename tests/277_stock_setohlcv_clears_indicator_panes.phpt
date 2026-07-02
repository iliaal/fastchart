--TEST--
StockChart: setOhlcv() drops native (candle-derived) indicator panes
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: native indicators (RSI/MACD/ATR/...) are computed eagerly
 * from the candle buffer at add() time and sized to that candle count.
 * Replacing the candles with setOhlcv() left the pane in place, drawing
 * stale values against the new timestamps. setOhlcv() now clears these
 * candle-derived panes (like the price overlays), while caller-supplied
 * addIndicatorPane() data is kept. Use native <text> so pane names show. */

function rows(int $n, float $base): array {
    $r = [];
    for ($i = 0; $i < $n; $i++) {
        $c = $base + $i;
        $r[] = [1700000000 + $i * 86400, $c, $c + 1, $c - 1, $c + 0.5, 1000];
    }
    return $r;
}

/* Native RSI pane must vanish after a second setOhlcv(). */
$s = (new FastChart\StockChart(500, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setOhlcv(rows(40, 100))
    ->addRSI(14)
    ->setOhlcv(rows(40, 200))
    ->renderSvg();
echo "rsi_after_resetohlcv: ", (str_contains($s, 'RSI') ? 'PRESENT' : 'cleared'), "\n";

/* Re-adding after the new data works and shows again. */
$s2 = (new FastChart\StockChart(500, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setOhlcv(rows(40, 100))
    ->addRSI(14)
    ->setOhlcv(rows(40, 200))
    ->addRSI(14)
    ->renderSvg();
echo "rsi_after_readd: ", (str_contains($s2, 'RSI') ? 'present' : 'MISSING'), "\n";

/* Caller-supplied addIndicatorPane() survives setOhlcv(). */
$s3 = (new FastChart\StockChart(500, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setOhlcv(rows(20, 100))
    ->addIndicatorPane('CUSTOM', array_fill(0, 20, 5.0))
    ->setOhlcv(rows(20, 200))
    ->renderSvg();
echo "custom_after_resetohlcv: ", (str_contains($s3, 'CUSTOM') ? 'present' : 'MISSING'), "\n";

?>
--EXPECT--
rsi_after_resetohlcv: cleared
rsi_after_readd: present
custom_after_resetohlcv: present
