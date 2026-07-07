--TEST--
GaugeChart::setBackgroundImage() composites onto the canvas (was ignored)
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php

/* GaugeChart drew its canvas with an unconditional opaque rect, so
 * setBackgroundImage() was a silent no-op. Routing through
 * fastchart_paint_canvas_bg() now composites the image: the SVG carries
 * an <image> element and the raster corner shows the image color. */

$bg = imagecreatetruecolor(20, 20);
$pink = imagecolorallocate($bg, 0xFF, 0x00, 0xCC);
imagefilledrectangle($bg, 0, 0, 19, 19, $pink);
$path = sys_get_temp_dir() . '/fc_gauge_bg_331.png';
imagepng($bg, $path);

$svg = (new FastChart\GaugeChart(300, 220))
    ->setRange(0, 100)->setValue(50)
    ->setBackgroundImage($path)
    ->renderSvg();
echo 'svg_has_image: ', (strpos($svg, '<image') !== false ? 'yes' : 'no'), "\n";

$png = (new FastChart\GaugeChart(300, 220))
    ->setRange(0, 100)->setValue(50)
    ->setBackgroundImage($path)
    ->renderPng();
unlink($path);

$im = imagecreatefromstring($png);
$rgba = imagecolorat($im, 3, 3);
$r = ($rgba >> 16) & 0xFF; $g = ($rgba >> 8) & 0xFF; $b = $rgba & 0xFF;
echo 'corner_is_bg: ',
    ($r > 200 && $g < 50 && $b > 150 ? 'yes' : sprintf('no (#%02x%02x%02x)', $r, $g, $b)), "\n";

echo "done\n";
?>
--EXPECT--
svg_has_image: yes
corner_is_bg: yes
done
