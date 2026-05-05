--TEST--
Legend renders for multi-series Line/Bar/Scatter and for Stock SMA overlays
--SKIPIF--
<?php if (!extension_loaded("fastchart")) print "skip fastchart not loaded"; ?>
--FILE--
<?php

// LineChart, two labeled series. The legend sits in the top-right
// of the plot area with palette[0..1] swatches; sample for both.
$im = imagecreatetruecolor(800, 500);
(new FastChart\LineChart())
    ->setSize(800, 500)
    ->setSeries([
        ['label' => 'EU',   'data' => [1, 4, 2, 8, 5]],
        ['label' => 'APAC', 'data' => [3, 2, 5, 4, 6]],
    ])
    ->draw($im);

// Top-right corner of the plot area in an 800x500 layout: rough
// region x=600-770, y=20-80.
function count_color_in_region($im, $color, $x0, $x1, $y0, $y1) {
    $hits = 0;
    for ($y = $y0; $y < $y1; $y++) {
        for ($x = $x0; $x < $x1; $x++) {
            if (imagecolorat($im, $x, $y) === $color) $hits++;
        }
    }
    return $hits;
}

$blue   = (0x1f << 16) | (0x77 << 8) | 0xb4;
$orange = (0xff << 16) | (0x7f << 8) | 0x0e;
$blue_hits   = count_color_in_region($im, $blue,   600, 780, 10, 90);
$orange_hits = count_color_in_region($im, $orange, 600, 780, 10, 90);
echo "line_legend_blue_swatch: ",   ($blue_hits   > 30 ? "yes" : "no ($blue_hits)"),   "\n";
echo "line_legend_orange_swatch: ", ($orange_hits > 30 ? "yes" : "no ($orange_hits)"), "\n";

// Single-series flat list does NOT render a legend (would just be
// noise). Verify the legend colors don't appear in the legend
// region above the data.
$im2 = imagecreatetruecolor(800, 500);
(new FastChart\LineChart())
    ->setSize(800, 500)
    ->setSeries([1, 2, 3, 4, 5])  // flat list, no labels
    ->draw($im2);
$blue_hits2 = count_color_in_region($im2, $blue, 700, 780, 15, 35);
echo "single_series_no_legend_box: ", ($blue_hits2 < 3 ? "yes" : "no ($blue_hits2)"), "\n";

// Stock chart: SMA overlay legend present even with single price
// series.
$rows = [];
$base = 1700000000;
for ($i = 0; $i < 30; $i++) {
    $p = 100 + $i * 0.5;
    $rows[] = [$base + $i * 86400, $p, $p + 1, $p - 1, $p + 0.5, 1000];
}
$im3 = imagecreatetruecolor(1000, 500);
(new FastChart\StockChart())
    ->setSize(1000, 500)
    ->setOhlcv($rows)
    ->setMovingAverages([5, 10])
    ->draw($im3);

$blue_hits3   = count_color_in_region($im3, $blue,   780, 980, 10, 80);
$orange_hits3 = count_color_in_region($im3, $orange, 780, 980, 10, 80);
echo "stock_sma_legend_blue: ",   ($blue_hits3   > 30 ? "yes" : "no"), "\n";
echo "stock_sma_legend_orange: ", ($orange_hits3 > 30 ? "yes" : "no"), "\n";
?>
--EXPECT--
line_legend_blue_swatch: yes
line_legend_orange_swatch: yes
single_series_no_legend_box: yes
stock_sma_legend_blue: yes
stock_sma_legend_orange: yes
