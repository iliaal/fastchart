<?php

$bytes = (new FastChart\BubbleChart(500, 400))
    ->setPoints([
        [1.0, 2.0, 5,  0xFF0000],
        [3.0, 5.0, 12, 0x00CC00],
        [5.0, 8.0, 8,  0x0000FF],
        [7.0, 4.0, 15, 0xFFAA00],
    ])
    ->renderPng();
echo "renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

// No-color fallback uses palette series colors.
$bytes2 = (new FastChart\BubbleChart(500, 400))
    ->setPoints([[1, 1, 10], [2, 3, 15], [3, 2, 8]])
    ->renderPng();
echo "default_color: ", strlen($bytes2) > 1024 ? "ok" : "bad", "\n";

// Empty data error.
try {
    (new FastChart\BubbleChart(400, 300))->setPoints([])->renderPng();
    echo "empty: no throw\n";
} catch (\Error $e) {
    echo "empty: error ok\n";
}

// Edge color overrides ring color.
$bytes3 = (new FastChart\BubbleChart(400, 300))
    ->setEdgeColor(0x000000)
    ->setPoints([[1, 1, 20], [2, 2, 15]])
    ->renderPng();
$im = imagecreatefromstring($bytes3);
$black = 0;
for ($y = 0; $y < 300; $y++)
    for ($x = 0; $x < 400; $x++)
        if (imagecolorat($im, $x, $y) === 0x000000) $black++;
echo "edge_color: ", ($black > 20 ? "yes" : "no"), "\n";
?>
