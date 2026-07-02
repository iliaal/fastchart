--TEST--
Extreme finite inputs cannot reach undefined float-to-int casts at render time
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Setter-time isfinite() checks accept finite-but-extreme doubles whose
 * sums, differences, or radian conversions overflow to Inf/NaN at render
 * time and previously hit UB float-to-int casts (aborts under the UBSan
 * CI build). Covers fnd_c5ede6a3 (calendar), fnd_b597ecd0 (contour),
 * fnd_377bf618 (pareto), fnd_ab0cf66f + fnd_7afdece0 (polar),
 * fnd_0373a717 (sankey), fnd_685770be + fnd_26976cc5 (stock). */

function ok(string $svg): string {
    return (strlen($svg) > 200 && str_contains($svg, '</svg>')) ? 'ok' : 'BAD';
}

/* Calendar: ULP-degenerate range (vmax = vmin + 1.0 is a no-op at
 * DBL_MAX) and Inf-wide range both made frac NaN. */
echo "calendar_dblmax: ", ok((new FastChart\CalendarHeatmap(800, 160))
    ->setData(['2026-01-01' => 1.7e308])->renderSvg()), "\n";
echo "calendar_inf_range: ", ok((new FastChart\CalendarHeatmap(800, 160))
    ->setData(['2026-01-01' => -1.0e308, '2026-01-02' => 1.0e308])
    ->renderSvg()), "\n";

/* Contour filled mode: four finite cells sum to Inf in the average;
 * mixed-sign extremes make span Inf and tv NaN. */
echo "contour_sum_inf: ", ok((new FastChart\ContourChart(300, 200))
    ->setFilled(true)
    ->setGrid([[1.0e308, 1.0e308], [1.0e308, 1.0e308]])->renderSvg()), "\n";
echo "contour_mixed_sign: ", ok((new FastChart\ContourChart(300, 200))
    ->setFilled(true)
    ->setGrid([[-1.0e308, 1.0e308], [1.0e308, -1.0e308]])->renderSvg()), "\n";

/* Pareto: individually-finite bars summing past DBL_MAX now throw
 * instead of pushing NaN percentages into the cumulative line. */
try {
    (new FastChart\ParetoChart(400, 300))
        ->setBars([['value' => 1.5e308], ['value' => 1.5e308]])
        ->renderSvg();
    echo "pareto_overflow: NO THROW\n";
} catch (Error $e) {
    echo "pareto_overflow: ",
        (str_contains($e->getMessage(), 'overflow') ? 'throws' : 'WRONG MSG'), "\n";
}

/* Polar: finite-but-huge angles overflowed the a * M_PI multiply in the
 * line/area branch and in the addVectors overlay. */
echo "polar_huge_angle: ", ok((new FastChart\PolarChart(400, 400))
    ->setSeries([[1.0e308, 1.0], [45, 2.0], [90, 1.5]])->renderSvg()), "\n";
echo "polar_huge_vector: ", ok((new FastChart\PolarChart(400, 400))
    ->setSeries([[0, 1.0], [90, 2.0]])
    ->addVectors([['angle' => 1.0e308, 'radius' => 1.0,
                   'angle_to' => 90.0, 'radius_to' => 2.0]])
    ->renderSvg()), "\n";

/* Sankey: gap reservation exceeding avail_h used to flip px_per_unit to
 * an absolute 1.0 scale, letting a 1e18 link value reach the int casts. */
$nodes = [];
$links = [];
for ($i = 0; $i < 30; $i++) {
    $nodes[] = ['label' => "S$i"];
    $links[] = ['from' => $i, 'to' => 30, 'value' => ($i === 0) ? 1.0e18 : 1.0];
}
$nodes[] = ['label' => 'Sink'];
echo "sankey_squeezed: ", ok((new FastChart\SankeyChart(800, 200))
    ->setNodes($nodes)->setLinks($links)->renderSvg()), "\n";

/* Stock: all candles at PHP_INT_MAX overflowed the degenerate-domain
 * +1 fixup; a finite-but-huge icon x hit an unguarded zend_long cast. */
echo "stock_ts_saturated: ", ok((new FastChart\StockChart(600, 300))
    ->setOhlcv([[PHP_INT_MAX, 100.0, 110.0, 90.0, 105.0, 1000.0]])
    ->renderSvg()), "\n";

/* 1x1 red PNG, inlined so the sanitizer test run (no ext/gd) covers
 * this case too. */
$icon = tempnam(sys_get_temp_dir(), 'fc_icon_') . '.png';
file_put_contents($icon, base64_decode(
    'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8'
    . 'z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg=='));
echo "stock_icon_huge_x: ", ok((new FastChart\StockChart(600, 300))
    ->setOhlcv([
        [1000, 100.0, 110.0, 90.0, 105.0, 1000.0],
        [2000, 101.0, 111.0, 91.0, 106.0, 1000.0],
    ])
    ->addIconAt(1.0e300, 100.0, $icon)
    ->renderSvg()), "\n";
@unlink($icon);

?>
--EXPECT--
calendar_dblmax: ok
calendar_inf_range: ok
contour_sum_inf: ok
contour_mixed_sign: ok
pareto_overflow: throws
polar_huge_angle: ok
polar_huge_vector: ok
sankey_squeezed: ok
stock_ts_saturated: ok
stock_icon_huge_x: ok
