--TEST--
StockChart STYLE_VECTOR: climax-deque ring buffer overflow misclassifies later bars
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: fastchart_stock.c:262 — the monotone-decreasing deque
 * for the sliding-window max of climax_value (volume × (high−low))
 * uses a ring buffer of capacity baseT+1 = 11, where head==tail
 * means "empty". The push happens BEFORE the stale-front drop, so
 * after 11 successful pushes without tail-domination (i.e. a strictly
 * decreasing cv sequence) dq_tail wraps back to dq_head and the
 * deque is silently corrupted. From iteration 11 onward, the front
 * lookup sees head==tail and substitutes climax_max=0, then later
 * pushes overwrite stale slots and keep climax_max anchored to the
 * just-pushed value rather than the true window max. Result: every
 * subsequent bar trivially satisfies climax >= climax_max and the
 * classifier promotes them to climax-up/down (lime/fuchsia). */

/* Synthetic OHLCV designed to trigger the overflow:
 *   - bars 0..10: strictly DECREASING (high-low), constant volume,
 *     all bullish. Each push adds without tail-domination. Push 11
 *     wraps dq_tail to dq_head.
 *   - bars 11..18: tiny (high-low), still bullish. cv is much smaller
 *     than the true window max cv[2..10], so under a working deque
 *     these bars are NOT climaxes. Under the corrupted deque they
 *     all get reclassified as climaxes (lime). */
$rows = [];
for ($i = 0; $i < 11; $i++) {
    $hl_span = 11 - $i;                 /* 11, 10, 9, ..., 1 */
    $high = 101 + $hl_span;
    $low  = 100 - $hl_span;
    $rows[] = [1700000000 + $i * 86400, 100.0, $high, $low, 101.0, 100.0];
}
for ($i = 11; $i < 19; $i++) {
    $rows[] = [1700000000 + $i * 86400, 100.0, 102.0, 99.5, 101.0, 100.0];
}

$svg = (new FastChart\StockChart(900, 300))
    ->setOhlcv($rows)
    ->setCandleStyle(FastChart\Chart::STYLE_VECTOR)
    ->renderSvg();

$lime    = substr_count($svg, '#00E640');
$fuchsia = substr_count($svg, '#E600C0');
$climax_total = $lime + $fuchsia;

/* Under the FIX, only bar 0 is a "climax" (climax_max=0 at i=0 is a
 * separate, pre-existing detail unrelated to this bug — there's no
 * window to compare against on the first bar). All bullish, so
 * lime; each candle emits the color twice (wick line + body rect)
 * for ~2 occurrences. We allow [1, 8] to give breathing room.
 *
 * Under the BUG, bar 0 plus bars 11..18 (9 climax bars × 2 colors
 * per bar = 18 occurrences) flood the SVG with lime. */
echo "climax_color_count_low: ",
    ($climax_total <= 8 ? "ok ($climax_total)" : "BAD ($climax_total)"), "\n";

/* Sanity: SVG is well-formed and contains 19 candle bodies. */
echo "all_candles_emitted: ",
    (substr_count($svg, '<rect') >= 19 ? "ok" : "BAD"), "\n";

echo "done\n";
?>
--EXPECTF--
climax_color_count_low: ok (%d)
all_candles_emitted: ok
done
