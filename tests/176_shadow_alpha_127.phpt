--TEST--
shadow_alpha_to_255: libgd 127 (fully transparent) maps to alpha 0, not alpha 1
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: shadow_alpha_to_255 at fastchart_effects.c:103 used the
 * linear formula `255 - a * 2`, clamping the input to [0,127]. With
 * input 127 (libgd convention for "fully transparent") the output is
 * 1/255, not 0. The SVG drop-shadow then renders as
 * rgba(0,0,0,0.004) — a faint dot of black rather than truly
 * invisible. */

$svg = (new FastChart\BarChart(200, 200))
    ->setSeries([['data' => [10, 20, 30]]])
    ->setDropShadow(3, 3)
    ->setShadowAlpha(127)   /* user intent: invisible shadow */
    ->renderSvg();

/* The shadow's rgba string should be alpha 0 (or it should be
 * skipped entirely). The pre-fix output contains rgba(0,0,0,0.004)
 * which is the visible signature of the bug. */
$has_004 = (strpos($svg, 'rgba(0,0,0,0.004)') !== false);
echo "no_004_alpha: ", ($has_004 ? "BAD" : "ok"), "\n";

/* Sanity: at libgd alpha 0 (opaque), the shadow renders solid black. */
$svg_opaque = (new FastChart\BarChart(200, 200))
    ->setSeries([['data' => [10, 20, 30]]])
    ->setDropShadow(3, 3)
    ->setShadowAlpha(0)
    ->renderSvg();
echo "opaque_shadow_still_renders: ",
    (strpos($svg_opaque, '#000000') !== false ? "ok" : "BAD"), "\n";

/* Mid-range alpha=64 should still produce ~50% opacity (not
 * affected by the fix's edge-case handling). */
$svg_mid = (new FastChart\BarChart(200, 200))
    ->setSeries([['data' => [10, 20, 30]]])
    ->setDropShadow(3, 3)
    ->setShadowAlpha(64)
    ->renderSvg();
preg_match('#rgba\(0,0,0,(0\.\d+)\)#', $svg_mid, $m);
$mid_alpha = isset($m[1]) ? (float)$m[1] : -1;
echo "mid_alpha_around_0_5: ",
    ($mid_alpha > 0.3 && $mid_alpha < 0.7 ? "ok" : "BAD ($mid_alpha)"), "\n";

echo "done\n";
?>
--EXPECT--
no_004_alpha: ok
opaque_shadow_still_renders: ok
mid_alpha_around_0_5: ok
done
