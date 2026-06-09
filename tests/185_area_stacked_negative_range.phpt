--TEST--
AreaChart stacked mode: negative values widen the Y range instead of clamping
--EXTENSIONS--
fastchart
--FILE--
<?php

/* The stacked layer polygons span [cum, cum + v]; with mixed-sign
 * data the partial cumulative sums go negative, so the Y axis must
 * extend below zero. Pre-fix the range pass only tracked the final
 * per-category total with dmin pinned to 0: negative layers clamped
 * onto the baseline and no negative tick was ever emitted. */

function has_negative_tick(string $svg): bool {
    /* Native text mode keeps tick labels as real <text> elements. */
    return (bool) preg_match('/>-\d/', $svg);
}

/* Mixed-sign: partial sums per category are 5,-5 and 5,7 — the
 * axis must reach down to -5. */
$c = (new FastChart\AreaChart(400, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setStacked(true)
    ->setSeries([
        ['data' => [5, 5]],
        ['data' => [-10, 2]],
    ]);
var_dump(has_negative_tick($c->renderSvg()));

/* All-negative: pre-fix dmin(0) > dmax(negative) tripped the
 * degenerate fallback and the chart collapsed onto a fabricated
 * [0, 1] axis with no negative ticks. */
$c2 = (new FastChart\AreaChart(400, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setStacked(true)
    ->setSeries([
        ['data' => [-5, -3]],
        ['data' => [-1, -2]],
    ]);
var_dump(has_negative_tick($c2->renderSvg()));

/* All-positive stacked data must not regress: no negative ticks. */
$c3 = (new FastChart\AreaChart(400, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setStacked(true)
    ->setSeries([
        ['data' => [5, 5]],
        ['data' => [10, 2]],
    ]);
var_dump(has_negative_tick($c3->renderSvg()));

?>
--EXPECT--
bool(true)
bool(true)
bool(false)
