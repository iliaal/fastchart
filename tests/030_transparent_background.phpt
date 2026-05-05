--TEST--
setTransparentBackground preserves alpha channel in PNG output
--EXTENSIONS--
fastchart
--FILE--
<?php

// Render a transparent-background line chart and read it back via
// imagecreatefromstring; sample a corner pixel for transparency.
$bytes = (new FastChart\LineChart(400, 300))
    ->setSeries([1, 5, 3, 8])
    ->setTransparentBackground(true)
    ->renderPng();

$im = imagecreatefromstring($bytes);
imagealphablending($im, false);

// Top-left corner should be fully transparent (alpha = 127 in gd's
// 0..127 alpha space).
$rgba = imagecolorat($im, 5, 5);
$alpha = ($rgba >> 24) & 0x7F;
echo "corner_alpha: ", ($alpha === 127 ? "transparent" : "alpha=$alpha"), "\n";

// Plot area should still be opaque.
$rgba = imagecolorat($im, 200, 150);
$alpha_plot = ($rgba >> 24) & 0x7F;
echo "plot_area_alpha: ", ($alpha_plot === 0 ? "opaque" : "alpha=$alpha_plot"), "\n";

// Default (no transparency): corner is opaque white.
$bytes_op = (new FastChart\LineChart(400, 300))
    ->setSeries([1, 5, 3, 8])
    ->renderPng();
$im_op = imagecreatefromstring($bytes_op);
imagealphablending($im_op, false);
$rgba = imagecolorat($im_op, 5, 5);
$alpha_op = ($rgba >> 24) & 0x7F;
echo "default_alpha: ", ($alpha_op === 0 ? "opaque" : "alpha=$alpha_op"), "\n";
?>
--EXPECT--
corner_alpha: transparent
plot_area_alpha: opaque
default_alpha: opaque
