--TEST--
StockChart STYLE_VECTOR: bar 0 is not spuriously flagged as a climax
--EXTENSIONS--
fastchart
--INI--
asan.detect_leaks=0
--FILE--
<?php

/* With the trailing climax-max deque empty (bar 0, or after volume-less
 * stretches) climax_max stayed 0, so `climax >= climax_max` was trivially
 * true and the first bar was always painted with the climax color. The
 * classifier now requires a real windowed max (have_max) before the
 * climax-by-range test fires.
 *
 * Dataset: strictly decreasing volume, constant high-low span, all
 * bullish. No bar legitimately reaches 2x the trailing average, and
 * every bar's climax value is strictly below the previous window max,
 * so a correct classifier marks ZERO bars as climax. Under the bug bar
 * 0 is lime. */

$rows = [];
for ($i = 0; $i < 8; $i++) {
    $vol = 200 - $i * 15;                 /* 200,185,...,95 strictly down */
    $rows[] = [1700000000 + $i * 86400, 100.0, 102.0, 100.0, 101.0, (float)$vol];
}

$svg = (new FastChart\StockChart(600, 300))
    ->setOhlcv($rows)
    ->setCandleStyle(FastChart\Chart::STYLE_VECTOR)
    ->renderSvg();

$lime    = substr_count($svg, '#00E640');   /* buying-climax color */
$fuchsia = substr_count($svg, '#E600C0');   /* selling-climax color */

echo 'climax_colors: ', ($lime + $fuchsia === 0 ? 'none ok' : "BAD ($lime+$fuchsia)"), "\n";
echo 'candles_emitted: ', (substr_count($svg, '<rect') >= 8 ? 'ok' : 'BAD'), "\n";

echo "done\n";
?>
--EXPECT--
climax_colors: none ok
candles_emitted: ok
done
