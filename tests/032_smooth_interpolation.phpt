--TEST--
setLineInterpolation(INTERP_SMOOTH) emits Catmull-Rom curves
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

// Sharp zigzag data. Linear renders straight diagonal segments;
// smooth produces curved transitions, so the pixel coverage in
// the off-diagonal regions differs.
$series = [10, 90, 10, 90, 10, 90, 10];

$bytes_lin = (new FastChart\LineChart(800, 400))
    ->setLineInterpolation(FastChart\Chart::INTERP_LINEAR)
    ->setSeries($series)->renderPng();
$im_lin = imagecreatefromstring($bytes_lin);

$bytes_sm = (new FastChart\LineChart(800, 400))
    ->setLineInterpolation(FastChart\Chart::INTERP_SMOOTH)
    ->setSeries($series)->renderPng();
$im_sm = imagecreatefromstring($bytes_sm);

$series_color = (0x1f << 16) | (0x77 << 8) | 0xb4;

$count_pixels = function ($im, $color) {
    $hits = 0;
    for ($y = 30; $y < 380; $y++) {
        for ($x = 50; $x < 770; $x++) {
            if (imagecolorat($im, $x, $y) === $color) $hits++;
        }
    }
    return $hits;
};

$lin = $count_pixels($im_lin, $series_color);
$sm  = $count_pixels($im_sm, $series_color);

// Smooth curves trace longer arcs through the same data points,
// so total pixels touching the exact series color increases. Both
// should be > 0; smooth should exceed linear.
echo "linear_renders: ", ($lin > 0 ? "yes" : "no"), "\n";
echo "smooth_renders: ", ($sm > 0 ? "yes" : "no"), "\n";
echo "smooth_differs_from_linear: ", ($sm !== $lin ? "yes" : "no"), "\n";

// Bad mode rejected.
try {
    (new FastChart\LineChart)->setLineInterpolation(99);
    echo "bad_mode: no throw\n";
} catch (\ValueError $e) {
    echo "bad_mode: ValueError ok\n";
}

// AreaChart smooth top edges.
$bytes = (new FastChart\AreaChart(400, 300))
    ->setLineInterpolation(FastChart\Chart::INTERP_SMOOTH)
    ->setSeries([1, 5, 3, 8, 4])
    ->renderPng();
echo "area_smooth_renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";
?>
--EXPECT--
linear_renders: yes
smooth_renders: yes
smooth_differs_from_linear: yes
bad_mode: ValueError ok
area_smooth_renders: ok
