--TEST--
AreaChart gradient honors setFillOpacity on non-stacked overlay
--EXTENSIONS--
fastchart
--FILE--
<?php

/* CR-007: previously the AreaChart gradient branch forced fully
 * opaque stops, so a non-stacked overlay with gradient on lost
 * the translucency that setFillOpacity (alias setAreaAlpha) gives
 * solid-fill overlays. The fix composes the alpha byte into the
 * gradient stops so layered shapes show through. */

/* Translucent (default alpha = 64 in gd 0..127). */
$svg_tx = (new FastChart\AreaChart(200, 120))
    ->setSeries([
        ['data' => [1, 3, 2, 5, 4]],
        ['data' => [2, 2, 4, 3, 5]],
    ])
    ->setGradientFill(0xFF0000, 0x00FF00)
    ->setFillOpacity(64)
    ->renderSvg();
echo "translucent_uses_rgba_stop: ",
    (preg_match('/<stop[^>]+stop-color="rgba\(/', $svg_tx)
        ? "yes" : "no"), "\n";
$tx_count = preg_match_all('/<linearGradient/', $svg_tx);
echo "translucent_gradient_count: $tx_count\n";

/* Opaque (alpha = 0 -> alpha_byte = 0xFF). */
$svg_op = (new FastChart\AreaChart(200, 120))
    ->setSeries([['data' => [1, 3, 2, 5, 4]]])
    ->setGradientFill(0xFF0000, 0x00FF00)
    ->setFillOpacity(0)
    ->renderSvg();
echo "opaque_uses_hex_stop: ",
    (preg_match('/<stop[^>]+stop-color="#[A-F0-9]{6}"/', $svg_op)
        ? "yes" : "no"), "\n";

/* Stacked AreaChart stays opaque regardless of setFillOpacity —
 * stacked is documented as always opaque. Gradient still applies. */
$svg_st = (new FastChart\AreaChart(200, 120))
    ->setSeries([
        ['data' => [1, 3, 2, 5, 4]],
        ['data' => [2, 2, 4, 3, 5]],
    ])
    ->setStacked(true)
    ->setGradientFill(0xFF0000, 0x00FF00)
    ->setFillOpacity(64)
    ->renderSvg();
echo "stacked_uses_hex_stop: ",
    (preg_match('/<stop[^>]+stop-color="#[A-F0-9]{6}"/', $svg_st)
        ? "yes" : "no"), "\n";

/* BarChart gradient (no alpha setter on bars) stays opaque —
 * verifies the default-to-0xFF logic in fc_svg_emit_gradient_def
 * for callers passing bare 24-bit RGB. */
$svg_bar = (new FastChart\BarChart(200, 120))
    ->setSeries([1, 3, 2, 5])
    ->setGradientFill(0xFF0000, 0x00FF00)
    ->renderSvg();
echo "bar_uses_hex_stop: ",
    (preg_match('/<stop[^>]+stop-color="#[A-F0-9]{6}"/', $svg_bar)
        ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
translucent_uses_rgba_stop: yes
translucent_gradient_count: 2
opaque_uses_hex_stop: yes
stacked_uses_hex_stop: yes
bar_uses_hex_stop: yes
ok
