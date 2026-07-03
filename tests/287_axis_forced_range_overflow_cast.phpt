--TEST--
Forced Y-axis range with an overflowing span no longer reaches (int)NaN
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression (fnd_0a5ecbbf): setYAxisRange with near-DBL_MAX bounds makes
 * the range span overflow to +Inf, so (v-min)/span in fastchart_y_to_pixel
 * becomes NaN. NaN passes both frac clamps and reaches (int)NaN — a
 * float-cast-overflow UB the CI UBSan build traps. The guard must clamp
 * instead, so the output holds only sane, in-canvas integer coordinates
 * (the bug emitted the INT_MIN-derived value -2147483385). */

/* The (int)NaN bug emitted the INT_MIN-derived coordinate -2147483385.
 * Legitimate coordinates sit inside the canvas; only hex colors are large
 * and those are always positive, so scan for an absurd negative token. */
function min_coord(string $svg): int {
    preg_match_all('/-\d+/', $svg, $m);
    $min = 0;
    foreach ($m[0] as $v) { if ((int)$v < $min) $min = (int)$v; }
    return $min;
}

$c = new FastChart\LineChart(400, 300);
$c->setSeries([['name' => 's', 'data' => [1.7e308, 1e307, 5e307]]]);
$c->setYAxisRange(-1.7e308, 1.7e308);
$svg = $c->renderSvg();
echo "renders: ", strlen($svg) > 100 ? "ok" : "BAD", "\n";
echo "no garbage coords: ", min_coord($svg) > -1000 ? "ok" : "BAD", "\n";

/* Same range on the raster path must not crash either. */
$png = (new FastChart\LineChart(200, 150))
    ->setSeries([['name' => 's', 'data' => [1e308, 2e307]]])
    ->setYAxisRange(-1e308, 1e308)
    ->renderPng();
echo "raster: ", strlen($png) > 50 ? "ok" : "BAD", "\n";

?>
--EXPECT--
renders: ok
no garbage coords: ok
raster: ok
