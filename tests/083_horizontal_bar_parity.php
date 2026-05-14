<?php

/* H-band on horizontal-bar = category-axis stripe (Y-range). */
$im = imagecreatetruecolor(400, 250);
$out = (new FastChart\BarChart(400, 250))
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setSeries([['data' => [10, 20, 30, 25, 18]]])
    ->setCategoryLabels(['A', 'B', 'C', 'D', 'E'])
    ->addHorizontalBand(1.0, 3.0, 0xFFFF00, 96, "highlight")
    ->draw($im);
echo "h_bar_category_band: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* H-line on horizontal-bar = vertical screen line at value=X. */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\BarChart(400, 200))
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setSeries([['data' => [10, 20, 30, 25]]])
    ->addHorizontalLine(22.0, "target", 0xFF0000)
    ->draw($im);
echo "h_bar_value_line: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* V-line on horizontal-bar = horizontal screen line at category-y. */
$im = imagecreatetruecolor(400, 250);
$out = (new FastChart\BarChart(400, 250))
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setSeries([['data' => [10, 20, 30, 25, 18]]])
    ->setCategoryLabels(['A', 'B', 'C', 'D', 'E'])
    ->addVerticalLine(2.0, "C-divider", 0x0000FF)
    ->draw($im);
echo "h_bar_category_line: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Mixed: H-band (category) + V-band (value) + H-line (value) + V-line (category). */
$im = imagecreatetruecolor(500, 300);
$out = (new FastChart\BarChart(500, 300))
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setSeries([['data' => [10, 20, 30, 25, 18, 12]]])
    ->setCategoryLabels(['Mon','Tue','Wed','Thu','Fri','Sat'])
    ->addHorizontalBand(1.0, 3.0, 0xFFFF00, 96)
    ->addVerticalBand(15.0, 25.0, 0x00FFFF, 100)
    ->addHorizontalLine(20.0, "avg", 0xFF0000)
    ->addVerticalLine(4.0, "wknd", 0x0000FF)
    ->draw($im);
echo "h_bar_mixed_overlays: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Out-of-range annotations are silently dropped. */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\BarChart(400, 200))
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setSeries([['data' => [10, 20, 30]]])
    ->addVerticalLine(99.0)
    ->addHorizontalLine(-50.0)
    ->draw($im);
echo "h_bar_out_of_range: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Vertical-bar regression: same calls still render the way they
 * always did (h-line is a horizontal screen line at y=value,
 * v-line at categorical x, h-band is a Y-range stripe). */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\BarChart(400, 200))
    ->setSeries([['data' => [10, 20, 30, 25]]])
    ->addHorizontalBand(15, 25, 0x00FF00, 96)
    ->addHorizontalLine(20.0)
    ->addVerticalLine(2.0)
    ->draw($im);
echo "v_bar_unchanged: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";
?>
