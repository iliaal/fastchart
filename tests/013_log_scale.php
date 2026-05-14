<?php

// Log-scale chart with values across 4 decades.
$im = imagecreatetruecolor(600, 400);
$out = (new FastChart\LineChart)
    ->setSize(600, 400)
    ->setYAxisScale(FastChart\Chart::SCALE_LOG)
    ->setSeries([1, 10, 100, 1000, 10000])
    ->draw($im);
var_dump($out instanceof \GdImage);

// On a log axis [1..10000], the value 100 lands at the midpoint
// of the plot's vertical span (log10(100) is the middle of
// [log10(1), log10(10000)]). Sample series-color pixels in the
// vertical middle band; with linear scale, value 100 would land
// near the bottom edge instead.
$series0 = (0x1f << 16) | (0x77 << 8) | 0xb4;
$mid_band_hits = 0;
$bottom_band_hits = 0;
for ($y = 180; $y < 220; $y++) {
    for ($x = 50; $x < 580; $x++) {
        if (imagecolorat($im, $x, $y) === $series0) $mid_band_hits++;
    }
}
for ($y = 350; $y < 390; $y++) {
    for ($x = 50; $x < 580; $x++) {
        if (imagecolorat($im, $x, $y) === $series0) $bottom_band_hits++;
    }
}
echo "log_value_100_near_middle: ", ($mid_band_hits > 5 ? "yes" : "no ($mid_band_hits)"), "\n";

// Non-positive data with log scale: ValueError.
try {
    $im = imagecreatetruecolor(400, 300);
    (new FastChart\LineChart)
        ->setSize(400, 300)
        ->setYAxisScale(FastChart\Chart::SCALE_LOG)
        ->setSeries([1, 10, 0, 100])
        ->draw($im);
    echo "log_zero: no throw\n";
} catch (\ValueError $e) {
    echo "log_zero: ValueError ok\n";
}

try {
    $im = imagecreatetruecolor(400, 300);
    (new FastChart\LineChart)
        ->setSize(400, 300)
        ->setYAxisScale(FastChart\Chart::SCALE_LOG)
        ->setSeries([-1, 1, 10])
        ->draw($im);
    echo "log_negative: no throw\n";
} catch (\ValueError $e) {
    echo "log_negative: ValueError ok\n";
}

// Bad scale enum rejected.
try {
    (new FastChart\LineChart)->setYAxisScale(7);
    echo "bad_scale: no throw\n";
} catch (\ValueError $e) {
    echo "bad_scale: ValueError ok\n";
}
?>
