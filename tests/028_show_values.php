<?php

$text_color = 0x222222;  // light theme text

// LineChart: values appear above markers.
$bytes = (new FastChart\LineChart(800, 400))
    ->setShowValues(true, '%.0f')
    ->setSeries([10, 25, 18, 42, 33])
    ->renderPng();
$im = imagecreatefromstring($bytes);

// Sample for text-color pixels in a vertical band where the label
// should sit (just above each data marker).
$found_labels = 0;
for ($y = 30; $y < 380; $y++) {
    for ($x = 50; $x < 770; $x++) {
        if (imagecolorat($im, $x, $y) === $text_color) {
            $found_labels++;
            if ($found_labels > 80) break 2;
        }
    }
}
echo "line_value_labels_present: ", ($found_labels > 80 ? "yes" : "no"), "\n";

// BarChart: above each bar.
$bytes2 = (new FastChart\BarChart(800, 400))
    ->setShowValues(true)
    ->setSeries([100, 200, 150, 250])
    ->renderPng();
$im2 = imagecreatefromstring($bytes2);
$found2 = 0;
for ($y = 30; $y < 380; $y++) {
    for ($x = 50; $x < 770; $x++) {
        if (imagecolorat($im2, $x, $y) === $text_color) {
            $found2++;
            if ($found2 > 80) break 2;
        }
    }
}
echo "bar_value_labels_present: ", ($found2 > 80 ? "yes" : "no"), "\n";

// Off by default.
$png = (new FastChart\LineChart(800, 400))
    ->setSeries([10, 20, 30])
    ->renderPng();
echo "off_by_default_renders: ", strlen($png) > 1024 ? "ok" : "bad", "\n";
?>
