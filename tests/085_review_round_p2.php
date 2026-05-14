<?php

/* Caps quietly clamp oversized inputs to the documented max. The
 * setter completes without error and the renderer accepts what
 * survives. (Strict-mode rejection is a separate setSeries-only
 * contract.) */

/* setVolumeColors clamps to FASTCHART_MAX_VOLUME_COLORS (= 4096). */
$rows = [];
$base = 1700000000;
for ($i = 0; $i < 30; $i++) {
    $p = 100 + $i;
    $rows[] = [$base + $i * 86400, $p, $p + 0.5, $p - 0.5, $p + 0.2, 1000];
}
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\StockChart(400, 200))
    ->setOhlcv($rows)
    ->setVolumeColors(array_fill(0, 5000, 0xFF0000))   // > cap, clamps
    ->draw($im);
echo "volume_colors_clamp: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* setExplode clamps to FASTCHART_MAX_SLICES (= 32). */
$im = imagecreatetruecolor(300, 300);
$out = (new FastChart\PieChart(300, 300))
    ->setSlices(['A' => 1, 'B' => 2, 'C' => 3])
    ->setExplode(array_fill(0, 100, 5))
    ->draw($im);
echo "explode_clamp: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* setLevels clamps to FASTCHART_MAX_LEVELS. */
$grid = [];
for ($r = 0; $r < 10; $r++) {
    $row = [];
    for ($c = 0; $c < 10; $c++) $row[] = sin($r / 3) + cos($c / 3);
    $grid[] = $row;
}
$im = imagecreatetruecolor(400, 300);
$out = (new FastChart\ContourChart(400, 300))
    ->setGrid($grid)
    ->setLevels(array_fill(0, 100, 0.5))
    ->draw($im);
echo "levels_clamp: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* setCategoryLabels clamps to FASTCHART_MAX_CATEGORY_LABELS. */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\LineChart(400, 200))
    ->setSeries([['data' => [10, 20, 30]]])
    ->setCategoryLabels(array_fill(0, 5000, "x"))
    ->draw($im);
echo "category_labels_clamp: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* BoxPlot outliers clamp to FASTCHART_MAX_OUTLIERS (= 128). */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\BoxPlot(400, 200))
    ->setBoxes([[
        'min' => 1, 'q1' => 3, 'median' => 5, 'q3' => 7, 'max' => 9,
        'outliers' => array_fill(0, 1000, 0.5),
    ]])
    ->draw($im);
echo "outliers_clamp: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Color-cache hot path: scatter with many points, mostly repeated
 * colors. Smoke-test: just verify it renders. */
$pts = [];
$colors = [0xFF0000, 0x00FF00, 0x0000FF];
for ($i = 0; $i < 500; $i++) {
    $pts[] = [$i, sin($i / 10) * 50, 'color' => $colors[$i % 3]];
}
$im = imagecreatetruecolor(800, 400);
$out = (new FastChart\ScatterChart(800, 400))
    ->setPoints($pts)
    ->draw($im);
echo "scatter_color_cache: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* renderToFile short-write detection: write to a real path, verify
 * success returns the byte count. (Triggering an actual short write
 * requires a filesystem-error injection we don't have; this just
 * verifies the success path still works.) */
$tmp = tempnam(sys_get_temp_dir(), 'fc_') . '.png';
$bytes = (new FastChart\LineChart(200, 100))
    ->setSeries([['data' => [1, 2, 3]]])
    ->renderToFile($tmp);
$ok = is_int($bytes) && $bytes > 0 && filesize($tmp) === $bytes;
@unlink($tmp);
echo "render_to_file_complete: ", ($ok ? "ok" : "bad"), "\n";

/* setStrict docstring update: confirm the constant exists and the
 * setter accepts both true and false without throwing. */
echo "strict_setter: ", (
    ((new FastChart\LineChart)->setStrict(true) instanceof FastChart\Chart) &&
    ((new FastChart\LineChart)->setStrict(false) instanceof FastChart\Chart)
    ? "ok" : "bad"
), "\n";
?>
