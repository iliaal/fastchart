--TEST--
BarChart::draw renders bars in the plot area; stacked toggles independently
--EXTENSIONS--
fastchart
--FILE--
<?php

// Single-series bars.
$im = imagecreatetruecolor(800, 600);
(new FastChart\BarChart())
    ->setSize(800, 600)
    ->setTitle('Bars')
    ->setSeries([5, 12, 8, 15, 10])
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
echo "single_bar_hits>30: ", ($hits > 30 ? "yes" : "no ($hits)"), "\n";

// Multi-series grouped.
$im2 = imagecreatetruecolor(800, 600);
(new FastChart\BarChart())
    ->setSize(800, 600)
    ->setSeries([
        ['label' => 'A', 'data' => [5, 8, 6, 10]],
        ['label' => 'B', 'data' => [4, 6, 9, 7]],
    ])
    ->draw($im2);

$series1 = (0xff << 16) | (0x7f << 8) | 0x0e;  // orange
$hits1 = 0;
for ($y = 80; $y < 520; $y += 3) {
    for ($x = 80; $x < 720; $x += 3) {
        if (imagecolorat($im2, $x, $y) === $series1) {
            $hits1++;
        }
    }
}
echo "grouped_series1_hits>10: ", ($hits1 > 10 ? "yes" : "no ($hits1)"), "\n";

// Stacked bars with negative values.
$im3 = imagecreatetruecolor(800, 600);
(new FastChart\BarChart())
    ->setSize(800, 600)
    ->setSeries([
        ['label' => 'A', 'data' => [5, 8, 6, 10]],
        ['label' => 'B', 'data' => [-2, 3, -4, 5]],
    ])
    ->setStacked(true)
    ->draw($im3);

$bg = imagecolorat($im3, 5, 5);
printf("bg=#%06x\n", $bg);

ob_start(); imagepng($im); $png = ob_get_clean();
echo "png_sig: ", substr(bin2hex($png), 0, 8) === '89504e47' ? "ok" : "bad", "\n";
?>
--EXPECT--
single_bar_hits>30: yes
grouped_series1_hits>10: yes
bg=#ffffff
png_sig: ok
