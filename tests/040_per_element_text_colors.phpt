--TEST--
setTitleColor / setAxisLabelColor / setAxisTitleColor override per-element text
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

$bytes = (new FastChart\LineChart(600, 400))
    ->setTitle('A title')
    ->setXAxisTitle('X')
    ->setYAxisTitle('Y')
    ->setTitleColor(0xFF0000)        // red title
    ->setAxisTitleColor(0x00AA00)    // green axis titles
    ->setAxisLabelColor(0x0000FF)    // blue tick labels
    ->setSeries([1, 5, 3, 8])
    ->renderPng();
$im = imagecreatefromstring($bytes);

$has_color = function ($im, $color, $w, $h) {
    for ($y = 0; $y < $h; $y++)
        for ($x = 0; $x < $w; $x++)
            if (imagecolorat($im, $x, $y) === $color) return true;
    return false;
};

echo "title_red: ", $has_color($im, 0xFF0000, 600, 400) ? "yes" : "no", "\n";
echo "axis_title_green: ", $has_color($im, 0x00AA00, 600, 400) ? "yes" : "no", "\n";
echo "axis_label_blue: ", $has_color($im, 0x0000FF, 600, 400) ? "yes" : "no", "\n";

// Out-of-range rejected.
try {
    (new FastChart\LineChart)->setTitleColor(0x1000000);
    echo "bad: no throw\n";
} catch (\ValueError $e) {
    echo "bad: ValueError ok\n";
}
?>
--EXPECT--
title_red: yes
axis_title_green: yes
axis_label_blue: yes
bad: ValueError ok
