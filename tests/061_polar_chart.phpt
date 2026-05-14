--TEST--
PolarChart: continuous angular plot with [angle, radius] points
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

// Single-series flat list of [angle, r].
$bytes = (new FastChart\PolarChart(500, 500))
    ->setSeries([[0, 1], [45, 2], [90, 3], [135, 2.5],
                 [180, 1.5], [225, 2], [270, 2.8], [315, 1.8]])
    ->renderPng();
echo "single_renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

// Multi-series with colors.
$bytes2 = (new FastChart\PolarChart(500, 500))
    ->setSeries([
        ['data' => [[0, 1], [120, 2], [240, 1.5]], 'label' => 'A', 'color' => 0xFF0000],
        ['data' => [[60, 1.5], [180, 2.5], [300, 2]], 'label' => 'B', 'color' => 0x00CC00],
    ])
    ->renderPng();
$im = imagecreatefromstring($bytes2);
$has = function ($im, $c, $w, $h) {
    for ($y = 0; $y < $h; $y++)
        for ($x = 0; $x < $w; $x++)
            if (imagecolorat($im, $x, $y) === $c) return true;
    return false;
};
echo "red_present: ",   $has($im, 0xFF0000, 500, 500) ? "yes" : "no", "\n";
echo "green_present: ", $has($im, 0x00CC00, 500, 500) ? "yes" : "no", "\n";

// Outline-only mode.
$bytes3 = (new FastChart\PolarChart(500, 500))
    ->setSeries([[0, 1], [90, 2], [180, 1], [270, 2]])
    ->setFilled(false)
    ->renderPng();
echo "outline: ", strlen($bytes3) > 1024 ? "ok" : "bad", "\n";

// Forced max radius.
$bytes4 = (new FastChart\PolarChart(500, 500))
    ->setSeries([[0, 0.5], [180, 0.8]])
    ->setMaxRadius(10.0)
    ->renderPng();
echo "max_radius: ", strlen($bytes4) > 1024 ? "ok" : "bad", "\n";
?>
--EXPECT--
single_renders: ok
red_present: yes
green_present: yes
outline: ok
max_radius: ok
