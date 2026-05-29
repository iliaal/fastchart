--TEST--
Area/Bar/Scatter icon coords clamped before float-to-int cast (regression: render UB)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Companion to 169 (LineChart). addIconAt() rejects only non-finite, so a
 * finite-but-huge coordinate left frac * plot_width past INT_MAX and the
 * subsequent (int) cast was undefined per C11 6.3.1.4p1 (on x86-64
 * cvttsd2si yields INT_MIN → <image x="-2147483648">). fastchart_line.c
 * clamped frac to [0,1]; area / bar (both orientations) / scatter skipped
 * that step. Uses the checked-in PNG fixture so the test needs no gd. */

$png = __DIR__ . '/__icon.png';

function ix($svg) { return preg_match('#<image\s+x="(-?\d+)"#', $svg, $m) ? (int)$m[1] : -987654321; }
function iy($svg) { return preg_match('#<image\s+x="-?\d+"\s+y="(-?\d+)"#', $svg, $m) ? (int)$m[1] : -987654321; }
function on_canvas($v) { return ($v >= -50 && $v <= 400) ? 'ok' : "BAD ($v)"; }

$area = (new FastChart\AreaChart(300, 200))
    ->setSeries([['data' => [1, 2, 3]]])->addIconAt(1e15, 1.0, $png)->renderSvg();
echo "area: ", on_canvas(ix($area)), "\n";

$bar = (new FastChart\BarChart(300, 200))
    ->setSeries([['data' => [1, 2, 3]]])->addIconAt(1e15, 1.0, $png)->renderSvg();
echo "bar_vertical: ", on_canvas(ix($bar)), "\n";

/* Horizontal bar exercises the frac_y clamp instead of frac_x. */
$barh = (new FastChart\BarChart(300, 200))
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setSeries([['data' => [1, 2, 3]]])->addIconAt(1.0, 1e15, $png)->renderSvg();
echo "bar_horizontal: ", on_canvas(iy($barh)), "\n";

$scatter = (new FastChart\ScatterChart(300, 200))
    ->setPoints([[1, 1], [2, 2], [3, 3]])->addIconAt(1e15, 1.0, $png)->renderSvg();
echo "scatter: ", on_canvas(ix($scatter)), "\n";

/* Sanity: an in-range icon still lands on-canvas. */
$ok = (new FastChart\AreaChart(300, 200))
    ->setSeries([['data' => [1, 2, 3]]])->addIconAt(1.0, 1.0, $png)->renderSvg();
echo "in_range: ", on_canvas(ix($ok)), "\n";
echo "done\n";
?>
--EXPECT--
area: ok
bar_vertical: ok
bar_horizontal: ok
scatter: ok
in_range: ok
done
