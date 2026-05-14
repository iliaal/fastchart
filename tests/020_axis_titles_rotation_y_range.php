<?php

$text_color = 0x222222;  // light theme text

// Axis titles render below X labels and rotated on Y.
$im = imagecreatetruecolor(900, 500);
(new FastChart\LineChart)
    ->setSize(900, 500)
    ->setXAxisTitle('Quarter')
    ->setYAxisTitle('Revenue (USD)')
    ->setSeries([100, 150, 200, 175, 220])
    ->draw($im);

// X-title should produce text pixels near the very bottom of the canvas.
$bottom_text_pixels = 0;
for ($y = 470; $y < 500; $y++) {
    for ($x = 100; $x < 800; $x++) {
        if (imagecolorat($im, $x, $y) === $text_color) $bottom_text_pixels++;
    }
}
echo "x_title_present: ", ($bottom_text_pixels > 30 ? "yes" : "no ($bottom_text_pixels)"), "\n";

// Y-title rotated -- look for text pixels in a narrow vertical band on the
// far left of the canvas where the rotated title sits.
$left_text_pixels = 0;
for ($y = 100; $y < 400; $y++) {
    for ($x = 5; $x < 30; $x++) {
        if (imagecolorat($im, $x, $y) === $text_color) $left_text_pixels++;
    }
}
echo "y_title_present: ", ($left_text_pixels > 30 ? "yes" : "no ($left_text_pixels)"), "\n";

// X-axis label rotation accepts 0, 45, 90 only.
foreach ([0, 45, 90] as $angle) {
    $im = imagecreatetruecolor(800, 500);
    (new FastChart\LineChart)->setSize(800, 500)
        ->setCategoryLabels(['January 2024', 'February 2024', 'March 2024', 'April 2024'])
        ->setXAxisLabelAngle($angle)
        ->setSeries([1, 4, 2, 8])->draw($im);
    echo "rotation_$angle: ok\n";
}

try {
    (new FastChart\LineChart)->setXAxisLabelAngle(30);
    echo "rotation_30: no throw\n";
} catch (\ValueError $e) {
    echo "rotation_30: ValueError ok\n";
}

// setYAxisRange: forced bounds reach the value_range.
$im = imagecreatetruecolor(800, 500);
(new FastChart\LineChart)->setSize(800, 500)
    ->setYAxisRange(0.0, 100.0, 25.0)
    ->setSeries([10, 50, 80, 30])->draw($im);
echo "y_range_force: ok\n";

// Forced min >= max rejected.
try {
    (new FastChart\LineChart)->setYAxisRange(100.0, 50.0);
    echo "y_range_inverted: no throw\n";
} catch (\ValueError $e) {
    echo "y_range_inverted: ValueError ok\n";
}

// Zero or negative interval rejected.
try {
    (new FastChart\LineChart)->setYAxisRange(0.0, 100.0, 0.0);
    echo "y_range_zero_int: no throw\n";
} catch (\ValueError $e) {
    echo "y_range_zero_int: ValueError ok\n";
}

// Render with forced range produces a valid PNG.
$bytes = (new FastChart\LineChart(400, 300))
    ->setYAxisRange(0.0, 200.0)
    ->setSeries([10, 50, 80])
    ->renderPng();
echo "y_range_render: ", substr(bin2hex($bytes), 0, 8) === '89504e47' ? "ok" : "bad", "\n";
?>
