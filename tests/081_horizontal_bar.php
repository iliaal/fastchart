<?php

/* Single-series horizontal bar. */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\BarChart(400, 200))
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setSeries([['data' => [10, 20, 30, 25, 18]]])
    ->setCategoryLabels(['A', 'B', 'C', 'D', 'E'])
    ->draw($im);
echo "single_horizontal: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Multi-series side-by-side. */
$im = imagecreatetruecolor(400, 300);
$out = (new FastChart\BarChart(400, 300))
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setSeries([
        ['label' => 'A', 'data' => [10, 20, 15]],
        ['label' => 'B', 'data' => [12, 18, 22]],
    ])
    ->setCategoryLabels(['x', 'y', 'z'])
    ->draw($im);
echo "multi_horizontal: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Stacked horizontal bar. */
$im = imagecreatetruecolor(400, 300);
$out = (new FastChart\BarChart(400, 300))
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setStacked(true)
    ->setSeries([
        ['data' => [10, 20, 30]],
        ['data' => [5,  10, 15]],
    ])
    ->draw($im);
echo "stacked_horizontal: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Floating horizontal bar (range plot). */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\BarChart(400, 200))
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setFloating(true)
    ->setSeries([['data' => [[10, 30], [15, 35], [5, 40]]]])
    ->setCategoryLabels(['Q1', 'Q2', 'Q3'])
    ->draw($im);
echo "floating_horizontal: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Negative values cross zero. */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\BarChart(400, 200))
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setSeries([['data' => [-10, 5, -15, 20, 8]]])
    ->draw($im);
echo "negatives_horizontal: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Default orientation is still vertical (no regression). */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\BarChart(400, 200))
    ->setSeries([['data' => [10, 20, 30]]])
    ->draw($im);
echo "default_vertical: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Validation: bogus orientation. */
try {
    (new FastChart\BarChart)->setOrientation(99);
    echo "bad_orientation: no throw\n";
} catch (\ValueError $e) {
    echo "bad_orientation: ValueError ok\n";
}

/* Constants are 0 / 1. */
echo "BAR_VERTICAL=", FastChart\BarChart::BAR_VERTICAL,
     " BAR_HORIZONTAL=", FastChart\BarChart::BAR_HORIZONTAL, "\n";
?>
