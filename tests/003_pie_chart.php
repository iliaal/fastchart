<?php

// Solid pie via assoc map.
$im = imagecreatetruecolor(600, 600);
(new FastChart\PieChart())
    ->setSize(600, 600)
    ->setTitle('Pie')
    ->setSlices(['A' => 50, 'B' => 25, 'C' => 25])
    ->draw($im);

// Center pixel of a solid pie should be one of the palette colors,
// not the white background. Sampled at the geometric center of the
// plot area which is within slice A (occupies the right half plus
// upper-left quadrant given start at -90deg).
$cx = 300;
$cy = 320;
$center = imagecolorat($im, $cx, $cy);
echo "center_is_white: ", ($center === 0xffffff ? "yes" : "no"), "\n";

$series0 = (0x1f << 16) | (0x77 << 8) | 0xb4;
$series1 = (0xff << 16) | (0x7f << 8) | 0x0e;
$series2 = (0x2c << 16) | (0xa0 << 8) | 0x2c;
$found = [false, false, false];
for ($y = 80; $y < 540; $y += 4) {
    for ($x = 60; $x < 540; $x += 4) {
        $c = imagecolorat($im, $x, $y);
        if ($c === $series0) $found[0] = true;
        elseif ($c === $series1) $found[1] = true;
        elseif ($c === $series2) $found[2] = true;
    }
}
echo "all_three_slices_visible: ",
    ($found[0] && $found[1] && $found[2] ? "yes" : "no"), "\n";

// Donut hole should overdraw the center with the plot background
// (white in light theme).
$im2 = imagecreatetruecolor(600, 600);
(new FastChart\PieChart())
    ->setSize(600, 600)
    ->setSlices(['A' => 50, 'B' => 25, 'C' => 25])
    ->setDonutHoleRatio(0.5)
    ->draw($im2);

$donut_center = imagecolorat($im2, 300, 300);
echo "donut_center: ", ($donut_center === 0xffffff ? "white" : sprintf("#%06x", $donut_center)), "\n";
?>
