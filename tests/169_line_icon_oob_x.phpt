--TEST--
LineChart icon x-coord clamped before float-to-int cast (regression: render UB)
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

/* Regression: fastchart_line.c:228 computes
 *   px = plot.x0 + (int)(frac_x * (plot.x1 - plot.x0) + 0.5);
 * where frac_x = (ic->x + 0.5) / max_len. addIconAt only rejects
 * non-finite, so a finite-but-large user value (e.g. 1e15) leaves
 * frac_x huge and the multiplication overflows past INT_MAX. The
 * subsequent float-to-int cast is undefined per C11 6.3.1.4p1; on
 * x86-64 the cvttsd2si instruction returns INT_MIN, so the SVG
 * gets <image x="-2147483634"/> instead of a clamped on-canvas
 * coord. The pixel-mapping helpers used elsewhere in this file
 * (fastchart_y_to_pixel) clamp frac to [0,1] before the cast for
 * exactly this reason; the icon path skips that step. */

$png = tempnam(sys_get_temp_dir(), 'fc_icon_') . '.png';
$im = imagecreatetruecolor(16, 16);
imagefill($im, 0, 0, imagecolorallocate($im, 0xff, 0x00, 0x00));
imagepng($im, $png);

$svg = (new FastChart\LineChart(200, 200))
    ->setSeries([['data' => [1, 2, 3]]])
    ->addIconAt(1e15, 1.0, $png)   /* x is wildly out of category range */
    ->renderSvg();

@unlink($png);

/* Find the <image .../> tag — it's the icon (background image not
 * configured). Pull out the integer x attribute. */
if (!preg_match('#<image\s+x="(-?\d+)"#', $svg, $m)) {
    echo "no_image_emitted: REGRESSION (icon not drawn at all)\n";
    exit;
}
$x = (int)$m[1];

/* The comment in fastchart_line.c says "out-of-range values still
 * render but get clipped to the plot edges". A clamped x lands near
 * plot.x1 (~170 for a 200px canvas). Without the clamp, the float
 * overflow lands the int cast in INT_MIN territory: x ≈ -2.1e9. */
echo "icon_x_within_canvas: ",
    ($x >= -50 && $x <= 250 ? "ok" : "BAD ($x)"), "\n";

/* Sanity: normal in-range x still renders on-canvas. */
$png2 = tempnam(sys_get_temp_dir(), 'fc_icon2_') . '.png';
$im2 = imagecreatetruecolor(16, 16);
imagefill($im2, 0, 0, imagecolorallocate($im2, 0x00, 0xff, 0x00));
imagepng($im2, $png2);

$svg2 = (new FastChart\LineChart(200, 200))
    ->setSeries([['data' => [1, 2, 3]]])
    ->addIconAt(1.0, 1.0, $png2)
    ->renderSvg();
@unlink($png2);

preg_match('#<image\s+x="(-?\d+)"#', $svg2, $m2);
$x2 = isset($m2[1]) ? (int)$m2[1] : -9999;
echo "in_range_icon_emitted: ",
    ($x2 >= 0 && $x2 <= 200 ? "ok" : "BAD ($x2)"), "\n";

echo "done\n";
?>
--EXPECT--
icon_x_within_canvas: ok
in_range_icon_emitted: ok
done
