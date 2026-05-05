--TEST--
ScatterChart::draw plots [x, y] pairs as colored points
--EXTENSIONS--
fastchart
--FILE--
<?php

$im = imagecreatetruecolor(800, 600);

// Single-series, 5 points spread across the plot.
(new FastChart\ScatterChart())
    ->setSize(800, 600)
    ->setTitle('Scatter')
    ->setPoints([
        [1.0, 2.0],
        [2.0, 5.0],
        [3.0, 3.5],
        [4.0, 8.0],
        [5.0, 6.0],
    ])
    ->draw($im);

$series0 = (0x1f << 16) | (0x77 << 8) | 0xb4;
$hits = 0;
for ($y = 80; $y < 520; $y += 3) {
    for ($x = 80; $x < 720; $x += 3) {
        if (imagecolorat($im, $x, $y) === $series0) {
            $hits++;
        }
    }
}
echo "scatter_dot_hits>5: ", ($hits > 5 ? "yes" : "no ($hits)"), "\n";

// Multi-series.
$im2 = imagecreatetruecolor(800, 600);
(new FastChart\ScatterChart())
    ->setPoints([
        ['label' => 'A', 'data' => [[0, 1], [1, 2], [2, 4], [3, 7]]],
        ['label' => 'B', 'data' => [[0, 5], [1, 4], [2, 3], [3, 1]]],
    ])
    ->draw($im2);

$series1 = (0xff << 16) | (0x7f << 8) | 0x0e;
$found_b = false;
for ($y = 80; $y < 520; $y += 2) {
    for ($x = 80; $x < 720; $x += 2) {
        if (imagecolorat($im2, $x, $y) === $series1) {
            $found_b = true;
            break 2;
        }
    }
}
echo "second_series_present: ", ($found_b ? "yes" : "no"), "\n";

ob_start(); imagepng($im); $png = ob_get_clean();
echo "png_sig: ", substr(bin2hex($png), 0, 8) === '89504e47' ? "ok" : "bad", "\n";
?>
--EXPECT--
scatter_dot_hits>5: yes
second_series_present: yes
png_sig: ok
