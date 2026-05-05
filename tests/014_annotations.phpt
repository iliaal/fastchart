--TEST--
addHorizontalLine / addVerticalLine: dashed reference lines + labels
--EXTENSIONS--
fastchart
--FILE--
<?php

// Horizontal annotation at y=50 on a [0..100] series with explicit
// red color. Should render dashed pixels in red across the plot.
$im = imagecreatetruecolor(800, 400);
(new FastChart\LineChart)
    ->setSize(800, 400)
    ->setSeries([0, 25, 50, 75, 100, 75, 50, 25, 0])
    ->addHorizontalLine(50.0, 'midline', 0xFF0000)
    ->draw($im);

// Sample for red pixels in a horizontal band where the y=50 line
// should land. The plot area is roughly y in [10, 380]; y=50 maps
// near the vertical middle around y=195.
$red_hits = 0;
for ($y = 180; $y < 220; $y++) {
    for ($x = 60; $x < 760; $x++) {
        if (imagecolorat($im, $x, $y) === 0xFF0000) $red_hits++;
    }
}
echo "horizontal_red_line_visible: ", ($red_hits > 30 ? "yes" : "no ($red_hits)"), "\n";

// Vertical annotation at category 4 (the peak) on a 9-point series.
$im2 = imagecreatetruecolor(800, 400);
(new FastChart\LineChart)
    ->setSize(800, 400)
    ->setSeries([0, 25, 50, 75, 100, 75, 50, 25, 0])
    ->addVerticalLine(4.0, 'peak', 0x008000)
    ->draw($im2);

// At category 4 of 9, the vertical line lands near the horizontal
// midpoint (just past it). Sample a vertical band there.
$green_hits = 0;
for ($y = 60; $y < 360; $y++) {
    for ($x = 380; $x < 440; $x++) {
        if (imagecolorat($im2, $x, $y) === 0x008000) $green_hits++;
    }
}
echo "vertical_green_line_visible: ", ($green_hits > 20 ? "yes" : "no ($green_hits)"), "\n";

// Multiple annotations on a single chart, no color override (uses
// theme axis color).
$im3 = imagecreatetruecolor(800, 400);
$out = (new FastChart\LineChart)
    ->setSize(800, 400)
    ->setSeries([0, 25, 50, 75, 100])
    ->addHorizontalLine(20.0)
    ->addHorizontalLine(80.0)
    ->addVerticalLine(2.0)
    ->draw($im3);
var_dump($out instanceof \GdImage);

// Bad color rejected.
try {
    (new FastChart\LineChart)->addHorizontalLine(1.0, null, 0xFFFFFFFF);
    echo "bad_color: no throw\n";
} catch (\ValueError $e) {
    echo "bad_color: ValueError ok\n";
}

// Stock annotations at timestamps.
$rows = [];
for ($i = 0; $i < 10; $i++) {
    $rows[] = [1700000000 + $i * 86400, 100 + $i, 102 + $i, 99 + $i, 101 + $i];
}
$im4 = imagecreatetruecolor(800, 400);
$out = (new FastChart\StockChart)
    ->setSize(800, 400)
    ->setOhlcv($rows)
    ->addHorizontalLine(105.0, '105', 0x0000FF)
    ->addVerticalLine((float)(1700000000 + 5 * 86400), 'event', 0xFF8000)
    ->draw($im4);
var_dump($out instanceof \GdImage);
?>
--EXPECT--
horizontal_red_line_visible: yes
vertical_green_line_visible: yes
bool(true)
bad_color: ValueError ok
bool(true)
