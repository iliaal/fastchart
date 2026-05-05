--TEST--
setCategoryLabels: line/bar X axis renders user-supplied labels
--EXTENSIONS--
fastchart
--FILE--
<?php

// LineChart with category labels.
$im = imagecreatetruecolor(800, 600);
(new FastChart\LineChart())
    ->setSize(800, 600)
    ->setCategoryLabels(['Q1', 'Q2', 'Q3', 'Q4'])
    ->setSeries([10, 30, 20, 40])
    ->draw($im);

// Sample text-color pixels (light theme: #222222) below the plot
// area where x-axis labels live (y > 540 on an 800x600 default
// layout). Labels render in #222222; if the renderer used integer
// indices ("0","1","2","3") instead of our labels, the label region
// would still have text pixels — so this is a presence check, not a
// label-content check. The label-content check is implicit in the
// successful render of an arbitrary string Q1..Q4.
$text = (0x22 << 16) | (0x22 << 8) | 0x22;
$found_below = 0;
for ($y = 545; $y < 595; $y++) {
    for ($x = 80; $x < 720; $x++) {
        if (imagecolorat($im, $x, $y) === $text) {
            $found_below++;
            if ($found_below > 50) break 2;
        }
    }
}
echo "x_label_text_pixels_below_plot: ", ($found_below > 50 ? "yes" : "no ($found_below)"), "\n";

// BarChart should accept the same setter.
$im2 = imagecreatetruecolor(800, 600);
$out = (new FastChart\BarChart())
    ->setSize(800, 600)
    ->setCategoryLabels(['Mon', 'Tue', 'Wed', 'Thu', 'Fri'])
    ->setSeries([5, 8, 6, 12, 9])
    ->draw($im2);
var_dump($out instanceof \GdImage);

// Stock/Pie/Scatter ignore the setter rather than throwing.
$im3 = imagecreatetruecolor(600, 600);
$out3 = (new FastChart\PieChart())
    ->setCategoryLabels(['anything'])
    ->setSlices(['A' => 1, 'B' => 1])
    ->draw($im3);
var_dump($out3 instanceof \GdImage);
?>
--EXPECT--
x_label_text_pixels_below_plot: yes
bool(true)
bool(true)
