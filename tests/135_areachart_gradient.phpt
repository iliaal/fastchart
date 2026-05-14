--TEST--
AreaChart::setGradientFill() emits linearGradient (stacked + non-stacked)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fastchart 1.0 documents setGradientFill() as supporting "bars, area
 * fills, pie slices" but the AreaChart polygon emit always called
 * fastchart_target_polygon() with the solid series color, ignoring
 * gradient_from / gradient_to. BarChart with the same setter emitted
 * <linearGradient> defs; AreaChart did not. The fix routes both the
 * stacked and non-stacked AreaChart fill paths through
 * fastchart_target_gradient_polygon() when a gradient is configured. */

/* Non-stacked. */
$svg1 = (new FastChart\AreaChart(200, 120))
    ->setSeries([1, 3, 2, 5, 4])
    ->setGradientFill(0xFF0000, 0x00FF00)
    ->renderSvg();
echo "non_stacked_gradient: ",
    (strpos($svg1, '<linearGradient') !== false ? "yes" : "no"), "\n";

/* Stacked — both layers should pick up the gradient. */
$svg2 = (new FastChart\AreaChart(200, 120))
    ->setSeries([
        ['data' => [1, 3, 2, 5, 4]],
        ['data' => [2, 2, 4, 3, 5]],
    ])
    ->setStacked(true)
    ->setGradientFill(0xFF0000, 0x00FF00)
    ->renderSvg();
$grad_count = substr_count($svg2, '<linearGradient');
echo "stacked_gradient_count: $grad_count\n";

/* Sanity: no gradient when the setter isn't called. */
$svg3 = (new FastChart\AreaChart(200, 120))
    ->setSeries([1, 3, 2, 5, 4])
    ->renderSvg();
echo "no_setter_gradient: ",
    (strpos($svg3, '<linearGradient') !== false ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
non_stacked_gradient: yes
stacked_gradient_count: 2
no_setter_gradient: no
ok
