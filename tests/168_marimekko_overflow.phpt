--TEST--
MarimekkoChart::setColumns(): non-finite running totals don't reach renderer (regression: int-cast UB)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: each individual segment value passes isfinite() and >0,
 * but the running sum col_total or cross-column total_w can overflow
 * to +Inf. The renderer at fastchart_marimekko.c computes
 *   col_frac = col->total / self->total_width   -- inf/inf = NaN
 *   col_w    = NaN * avail_w                    -- NaN
 *   x1       = plot_x0 + (int)(cx_acc + 0.5)    -- float-to-int UB
 *
 * Expected behavior after fix: silently drop columns whose totals
 * are non-finite (matches the existing val<=0 / skept==0 silent-skip).
 * Case 2 (all columns bad) leaves no usable column, so the renderer
 * throws "requires setColumns() with at least one positive segment". */

/* Reference: a single well-formed column renders 2 rects per segment
 * (fill + border) + 1 background rect. 2 segments → 5 rects. */

/* Case 1: per-column sum overflows on the FIRST column. After the fix
 * that column is dropped; only the second well-formed column renders. */
$svg1 = (new FastChart\MarimekkoChart(400, 300))
    ->setColumns([
        ['label' => 'Bad', 'segments' => [
            ['label' => 'A', 'value' => 1e308],
            ['label' => 'B', 'value' => 1e308],
        ]],
        ['label' => 'Good', 'segments' => [
            ['label' => 'A', 'value' => 10],
            ['label' => 'B', 'value' => 20],
        ]],
    ])
    ->renderSvg();
$rects1 = substr_count($svg1, '<rect');

/* With the bug: the bad column's NaN coords cascade through cx_acc,
 * producing a stream of clamped 1-pixel rects for every segment of
 * BOTH columns (4 segments × 2 rects = 8 + bg = 9, plus garbage from
 * NaN-driven x positions).
 *
 * With the fix: bad column dropped; only Good column's 2 segments
 * render = 4 segment rects + 1 bg = 5. */
echo "case1_rect_count_is_5: ", ($rects1 === 5 ? "ok" : "BAD ($rects1)"), "\n";

/* Case 2: every column has an Inf total, so after dropping them all
 * the renderer has no positive segments and throws. */
$threw = false;
try {
    (new FastChart\MarimekkoChart(400, 300))
        ->setColumns([
            ['label' => 'C1', 'segments' => [['value' => 1e308], ['value' => 1e308]]],
            ['label' => 'C2', 'segments' => [['value' => 1e308], ['value' => 1e308]]],
        ])
        ->renderSvg();
} catch (\Throwable $e) {
    $threw = strpos($e->getMessage(), 'at least one positive segment') !== false;
}
echo "case2_all_bad_throws: ", ($threw ? "ok" : "BAD"), "\n";

/* Case 3: cross-column total_w overflows. Each column individually has
 * col_total = 1e308 (finite), but 1e308 + 1e308 = 2e308 > DBL_MAX
 * overflows to +Inf. With the fix, only the first column survives;
 * adding either of the other two would overflow total_w, so they're
 * dropped. 1 segment × 2 rects + 1 bg = 3. */
$svg3 = (new FastChart\MarimekkoChart(400, 300))
    ->setColumns([
        ['label' => 'C1', 'segments' => [['value' => 1e308]]],
        ['label' => 'C2', 'segments' => [['value' => 1e308]]],
        ['label' => 'C3', 'segments' => [['value' => 1e308]]],
    ])
    ->renderSvg();
echo "case3_rect_count_is_3: ",
    (substr_count($svg3, '<rect') === 3 ? "ok" : "BAD (" . substr_count($svg3, '<rect') . ")"), "\n";

/* Sanity: finite values still work end-to-end. */
$svg4 = (new FastChart\MarimekkoChart(400, 300))
    ->setColumns([
        ['label' => 'Q1', 'segments' => [['value' => 30], ['value' => 20]]],
        ['label' => 'Q2', 'segments' => [['value' => 40], ['value' => 10]]],
    ])
    ->renderSvg();
echo "finite_still_works: ",
    (substr_count($svg4, '<rect') === 9 ? "ok" : "BAD (" . substr_count($svg4, '<rect') . ")"), "\n";

echo "done\n";
?>
--EXPECT--
case1_rect_count_is_5: ok
case2_all_bad_throws: ok
case3_rect_count_is_3: ok
finite_still_works: ok
done
