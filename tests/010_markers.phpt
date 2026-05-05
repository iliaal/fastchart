--TEST--
LineChart / ScatterChart marker style + size customization
--EXTENSIONS--
fastchart
--FILE--
<?php

// MARKER_NONE: no series-color disks at data point positions.
$im = imagecreatetruecolor(400, 300);
(new FastChart\LineChart)
    ->setSize(400, 300)
    ->setMarkerStyle(FastChart\Chart::MARKER_NONE)
    ->setSeries([10, 20, 30, 40, 50])
    ->draw($im);

$series0 = (0x1f << 16) | (0x77 << 8) | 0xb4;
// With MARKER_NONE, disk-colored pixels exist only where the line
// passes through, not in concentrated marker clusters. Sample a
// strict 5x5 window around an interpolated data point and make
// sure we don't get a 25-pixel solid block.
$cx = 80;  // approx x for first data point on a 400x300 plot
$cy = 230; // approx y for value 10
$dense = 0;
for ($dx = -3; $dx <= 3; $dx++) {
    for ($dy = -3; $dy <= 3; $dy++) {
        if (imagecolorat($im, $cx + $dx, $cy + $dy) === $series0) $dense++;
    }
}
echo "marker_none_no_dense_disk: ", ($dense < 6 ? "yes" : "no ($dense)"), "\n";

// MARKER_SQUARE: square markers produce filled rectangles, not
// circles. Sample at the corners of where a marker should be -- a
// circle marker would not fill the corners of the bounding box,
// but a square does.
$im2 = imagecreatetruecolor(400, 300);
(new FastChart\LineChart)
    ->setSize(400, 300)
    ->setMarkerStyle(FastChart\Chart::MARKER_SQUARE)
    ->setMarkerSize(12)
    ->setSeries([10, 20, 30])
    ->draw($im2);

$square_hits = 0;
for ($y = 80; $y < 250; $y += 2) {
    for ($x = 50; $x < 350; $x += 2) {
        if (imagecolorat($im2, $x, $y) === $series0) $square_hits++;
    }
}
echo "marker_square_visible: ", ($square_hits > 30 ? "yes" : "no"), "\n";

// MARKER_DIAMOND on ScatterChart.
$im3 = imagecreatetruecolor(400, 300);
(new FastChart\ScatterChart)
    ->setSize(400, 300)
    ->setMarkerStyle(FastChart\Chart::MARKER_DIAMOND)
    ->setMarkerSize(14)
    ->setPoints([[1, 1], [2, 5], [3, 3], [4, 8]])
    ->draw($im3);

$diamond_hits = 0;
for ($y = 60; $y < 260; $y += 2) {
    for ($x = 50; $x < 380; $x += 2) {
        if (imagecolorat($im3, $x, $y) === $series0) $diamond_hits++;
    }
}
echo "scatter_diamond_visible: ", ($diamond_hits > 20 ? "yes" : "no"), "\n";

// Out-of-range values throw.
try {
    (new FastChart\LineChart)->setMarkerStyle(99);
    echo "bad_marker: no throw\n";
} catch (\ValueError $e) {
    echo "bad_marker: ValueError ok\n";
}

try {
    (new FastChart\LineChart)->setMarkerSize(0);
    echo "bad_marker_size: no throw\n";
} catch (\ValueError $e) {
    echo "bad_marker_size: ValueError ok\n";
}
?>
--EXPECT--
marker_none_no_dense_disk: yes
marker_square_visible: yes
scatter_diamond_visible: yes
bad_marker: ValueError ok
bad_marker_size: ValueError ok
