--TEST--
ScatterChart::setTrendLine overlays a least-squares regression line
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

// Strongly linear data; trend should sit close to the points.
$points = [
    [1.0, 2.0], [2.0, 3.1], [3.0, 3.9],
    [4.0, 5.2], [5.0, 5.8], [6.0, 7.1], [7.0, 7.9],
];

$bytes = (new FastChart\ScatterChart(500, 400))
    ->setPoints($points)
    ->setTrendLine(true, 0xFF0000)
    ->renderPng();
$im = imagecreatefromstring($bytes);

// Red trend should appear in the plot.
$red = 0;
for ($y = 0; $y < 400; $y++)
    for ($x = 0; $x < 500; $x++)
        if (imagecolorat($im, $x, $y) === 0xFF0000) $red++;
echo "trend_red_present: ", ($red > 50 ? "yes" : "no"), "\n";

// Default color (axis color) when no override.
$bytes2 = (new FastChart\ScatterChart(500, 400))
    ->setPoints($points)
    ->setTrendLine(true)
    ->renderPng();
echo "default_color_renders: ", strlen($bytes2) > 1024 ? "ok" : "bad", "\n";

// Off by default: no extra red line.
$bytes3 = (new FastChart\ScatterChart(500, 400))
    ->setPoints($points)
    ->renderPng();
$im3 = imagecreatefromstring($bytes3);
$red3 = 0;
for ($y = 0; $y < 400; $y++)
    for ($x = 0; $x < 500; $x++)
        if (imagecolorat($im3, $x, $y) === 0xFF0000) $red3++;
echo "off_no_red: ", ($red3 === 0 ? "yes" : "no ($red3)"), "\n";

// Bad color.
try {
    (new FastChart\ScatterChart)->setTrendLine(true, 0x1000000);
    echo "bad: no throw\n";
} catch (\ValueError $e) {
    echo "bad: ValueError ok\n";
}
?>
--EXPECT--
trend_red_present: yes
default_color_renders: ok
off_no_red: yes
bad: ValueError ok
