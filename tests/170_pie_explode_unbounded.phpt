--TEST--
PieChart::setExplode(): unbounded offset clamped before float-to-int cast (regression: UB)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: fastchart_pie.c:153 computes
 *   slice_cx = cx + (int)((double)off * cos(mid_rad));
 *   slice_cy = cy + (int)((double)off * sin(mid_rad));
 * setExplode accepts any positive zend_long with no upper bound; a
 * value near PHP_INT_MAX (9.2e18) makes the inner double product
 * exceed INT_MAX, so the float-to-int cast is undefined per C11
 * 6.3.1.4. On x86-64 the conversion typically yields INT_MIN and
 * the SVG ends up with absurd negative path coords. Fix: cap to a
 * sane pixel range inside the renderer (mirrors the diameter-sized
 * physical limit). */

$normal = (new FastChart\PieChart())
    ->setSize(300, 300)
    ->setSlices([
        ['label' => 'A', 'value' => 30],
        ['label' => 'B', 'value' => 40],
        ['label' => 'C', 'value' => 30],
    ])
    ->renderSvg();

/* Verify the baseline emits path coords in canvas-positive territory. */
preg_match_all('/-?\d+/', $normal, $base_nums);
$base_min = min(array_map('intval', $base_nums[0]));
echo "baseline_no_huge_negative: ",
    ($base_min > -1000 ? "ok" : "BAD ($base_min)"), "\n";

/* Now with a pathological explode offset. */
$bad = (new FastChart\PieChart())
    ->setSize(300, 300)
    ->setSlices([
        ['label' => 'A', 'value' => 30],
        ['label' => 'B', 'value' => 40],
        ['label' => 'C', 'value' => 30],
    ])
    ->setExplode([0 => PHP_INT_MAX])
    ->renderSvg();

preg_match_all('/-?\d+/', $bad, $bad_nums);
$bad_min = min(array_map('intval', $bad_nums[0]));
$bad_max = max(array_map('intval', $bad_nums[0]));

/* With the bug: INT_MIN-region coords (~-2.1e9) leak into the path
 * data. With the fix: coords stay in the canvas-clamped range. */
echo "no_intmin_coords: ",
    ($bad_min > -100000 ? "ok" : "BAD ($bad_min)"), "\n";
echo "no_overflow_max:  ",
    ($bad_max < 1000000 ? "ok" : "BAD ($bad_max)"), "\n";

echo "done\n";
?>
--EXPECT--
baseline_no_huge_negative: ok
no_intmin_coords: ok
no_overflow_max:  ok
done
