<?php

// Per-bar colors in BarChart.
$im = imagecreatetruecolor(800, 400);
(new FastChart\BarChart)
    ->setSize(800, 400)
    ->setSeries([
        ['label' => 'X',
         'data'   => [10, 20, 15, 25, 30],
         'colors' => [0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xFF00FF]],
    ])
    ->draw($im);

$expected = [0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xFF00FF];
$found = [false, false, false, false, false];
for ($y = 40; $y < 360; $y += 2) {
    for ($x = 70; $x < 770; $x += 2) {
        $c = imagecolorat($im, $x, $y);
        foreach ($expected as $i => $want) {
            if ($c === $want) $found[$i] = true;
        }
    }
}
$count_found = count(array_filter($found));
echo "bar_ext_colors_present: ", ($count_found >= 4 ? "yes ($count_found/5)" : "no ($count_found/5)"), "\n";

// Per-point ext colors in LineChart with the MARKER_SQUARE style
// so the override is unambiguous (anti-aliased line strokes don't
// blend over the disk's center pixels and disappear the override).
$im2 = imagecreatetruecolor(800, 400);
(new FastChart\LineChart)
    ->setSize(800, 400)
    ->setMarkerStyle(FastChart\Chart::MARKER_SQUARE)
    ->setMarkerSize(18)
    ->setSeries([
        ['data'   => [10, 20, 15, 25, 30],
         'colors' => [0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xFF00FF]],
    ])
    ->draw($im2);

$found2 = [false, false, false, false, false];
for ($y = 0; $y < 400; $y += 1) {
    for ($x = 0; $x < 800; $x += 1) {
        $c = imagecolorat($im2, $x, $y);
        foreach ($expected as $i => $want) {
            if ($c === $want) $found2[$i] = true;
        }
    }
}
$count_found2 = count(array_filter($found2));
echo "line_marker_ext_colors_present: ", ($count_found2 >= 5 ? "yes (5/5)" : "no ($count_found2/5)"), "\n";

// Secondary Y axis: two series, one on each axis. Both should
// render despite the magnitude gap (price 100s vs volume millions).
$im3 = imagecreatetruecolor(900, 500);
$out = (new FastChart\LineChart)
    ->setSize(900, 500)
    ->setSecondaryYAxis(true)
    ->setSeries([
        ['label' => 'price',  'data' => [100, 110, 105, 120, 115], 'axis' => 'left'],
        ['label' => 'volume', 'data' => [1_000_000, 1_500_000, 800_000, 1_200_000, 950_000], 'axis' => 'right'],
    ])
    ->draw($im3);
var_dump($out instanceof \GdImage);

// Both palette colors should be visible inside the plot area.
$blue   = 0x1f77b4;
$orange = 0xff7f0e;
$found_blue = $found_orange = false;
for ($y = 40; $y < 460; $y += 2) {
    for ($x = 80; $x < 820; $x += 2) {
        $c = imagecolorat($im3, $x, $y);
        if ($c === $blue)   $found_blue = true;
        if ($c === $orange) $found_orange = true;
    }
}
echo "secondary_y_left_series_present: ",  $found_blue   ? "yes" : "no", "\n";
echo "secondary_y_right_series_present: ", $found_orange ? "yes" : "no", "\n";
?>
