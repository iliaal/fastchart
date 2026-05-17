--TEST--
Time-axis arithmetic: extreme zend_long timestamps don't trigger signed-overflow UB
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Four regressions in the same family:
 *   fnd_b5fe448f — fastchart_x_time_to_pixel: zend_long span = t_max - t_min
 *                  overflows when timestamps straddle the zend_long range
 *   fnd_1db3f20f — fastchart_draw_x_axis_time fallback ticks: same pattern
 *   fnd_5c7d3d47 — fastchart_draw_v_plot_bands_time: out-of-range double
 *                  cast to zend_long is UB per Annex F
 *   fnd_50d3eeb8 — Gantt renderer: t_max = t_min + 86400 overflows when
 *                  t_min == ZEND_LONG_MAX
 *
 * For each, the discriminator is "renders without crashing on extreme
 * input AND produces well-formed SVG." Without the fixes, the
 * arithmetic is UB; with them, the renderer either clamps to canvas
 * or returns a safe fallback. */

/* Case 1: stock chart with extreme-range timestamps. Hits both
 * fastchart_x_time_to_pixel (per candle) and the time-axis tick
 * generator (fallback branch).
 *
 * Discriminator: with the BUG, span = t_max - t_min overflows to
 * a small negative, the `span <= 0` early-return triggers, and
 * every candle defaults to plot.x0 → rects cluster on the left.
 * With the FIX, double arithmetic correctly spans the range and
 * the rightmost candle lands near plot.x1 (≈ 565 on a 600px
 * canvas). */
$rows = [];
$rows[] = [PHP_INT_MIN,         100.0, 110.0, 90.0, 105.0, 1000.0];
$rows[] = [PHP_INT_MIN + 1,     101.0, 111.0, 91.0, 106.0, 1000.0];
$rows[] = [PHP_INT_MAX - 1,     102.0, 112.0, 92.0, 107.0, 1000.0];
$rows[] = [PHP_INT_MAX,         103.0, 113.0, 93.0, 108.0, 1000.0];
$svg1 = (new FastChart\StockChart(600, 300))
    ->setOhlcv($rows)
    ->renderSvg();
preg_match_all('/<rect[^>]*x="(\d+)"[^>]*fill="#[A-F0-9]+"/', $svg1, $m);
$max_x = empty($m[1]) ? -1 : max(array_map('intval', $m[1]));
echo "candles_spread_across_canvas: ",
    ($max_x > 400 ? "ok" : "BAD (max_rect_x=$max_x — candles clustered left)"), "\n";

/* Case 2: vertical plot bands with finite-but-huge doubles
 * (1e30 — well outside the zend_long range). The pre-fix code
 * cast to zend_long unconditionally → UB. */
$rows2 = [];
for ($i = 0; $i < 5; $i++) {
    $rows2[] = [1700000000 + $i * 86400, 100.0, 110.0, 90.0, 105.0, 1000.0];
}
$svg2 = (new FastChart\StockChart(600, 300))
    ->setOhlcv($rows2)
    ->addVerticalBand(1e30, 1e30 + 1, 0xCCCCCC)
    ->renderSvg();
echo "extreme_band_renders: ",
    (strlen($svg2) > 1000 && strpos($svg2, '</svg>') !== false ? "ok" : "BAD"), "\n";

/* Case 3: Gantt with t_min == ZEND_LONG_MAX, no end. Renderer's
 * `t_max = t_min + 86400` overflows. */
$svg3 = (new FastChart\GanttChart(600, 300))
    ->setTasks([
        ['name' => 'A', 'start' => PHP_INT_MAX, 'end' => PHP_INT_MAX],
    ])
    ->renderSvg();
echo "extreme_gantt_renders: ",
    (strlen($svg3) > 500 && strpos($svg3, '</svg>') !== false ? "ok" : "BAD"), "\n";

/* Sanity: normal-range timestamps still produce the expected output. */
$rows4 = [];
for ($i = 0; $i < 4; $i++) {
    $rows4[] = [1700000000 + $i * 86400, 100.0, 110.0, 90.0, 105.0, 1000.0];
}
$svg4 = (new FastChart\StockChart(600, 300))
    ->setOhlcv($rows4)
    ->renderSvg();
echo "normal_timestamps_still_work: ",
    (strlen($svg4) > 1000 ? "ok" : "BAD"), "\n";

echo "done\n";
?>
--EXPECT--
candles_spread_across_canvas: ok
extreme_band_renders: ok
extreme_gantt_renders: ok
normal_timestamps_still_work: ok
done
