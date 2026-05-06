<?php
/* Catch-all for the remaining utility setters:
 *   - FastChart\Chart::version()       : extension version string (static)
 *   - setSize(w, h)                    : change canvas size after construction
 *     (constructor accepts the same args; setter is for already-built objects)
 *   - setPlotRect(x0, y0, x1, y1)      : pin the plot rectangle to fixed
 *     pixel coordinates instead of letting the layout helper pick. Useful
 *     when compositing multiple charts onto one canvas (alongside draw()).
 *   - setStrict(true)                  : reject non-numeric Line/Area/Bar
 *     setSeries cells with a TypeError instead of silently coercing to NaN
 *   - setBoxWidth(percent)             : boxplot box width as a percent of
 *     the per-category slot
 *   - setBackgroundImage(path)         : overlay a background image
 */

require __DIR__ . '/_bootstrap.php';

echo "fastchart version: ", FastChart\Chart::version(), "\n\n";

/* setSize override: equivalent to constructor args, useful in fluent
 * chains where the size is computed after the object is built. */
$chart = (new FastChart\LineChart())
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setSize(640, 320)
    ->setTitle('setSize after construction')
    ->setSeries([['data' => [10, 20, 30, 25, 18, 22, 30]]])
    ->setCategoryLabels(['Mon','Tue','Wed','Thu','Fri','Sat','Sun']);
$chart->renderToFile(__DIR__ . '/31a_setsize.png');

/* setPlotRect: explicit plot rectangle. Combined with draw() onto a
 * larger canvas, this lets you stack multiple charts on one image.
 *
 * Note: when setPlotRect is set, fastchart skips its canvas-wide bg
 * fill so it doesn't clobber neighbouring charts on the same image.
 * The caller pre-fills the canvas (here a soft #f5f5f5 backdrop). */
/* Note: don't call setDpi() on these two charts. With a caller-owned
 * canvas the chart can't resize the image; calling setDpi(200) on a
 * fixed 800x320 canvas would scale layout margins and FreeType up
 * while the canvas stays fixed and labels would overflow. setDpi()
 * *can* be used on the draw($canvas) path when the caller allocates
 * a proportionally larger canvas (see the stub doc note on draw);
 * we keep it off here because this canvas is fixed. */
$im = imagecreatetruecolor(800, 320);
$bg = imagecolorallocate($im, 0xF5, 0xF5, 0xF5);
imagefilledrectangle($im, 0, 0, 799, 319, $bg);
(new FastChart\LineChart(800, 320))
    ->setFontPath($font)
    ->setTitle('Left chart')
    ->setSeries([['data' => [22, 35, 28, 41, 38]]])
    ->setPlotRect(60, 30, 380, 280)
    ->draw($im);
(new FastChart\BarChart(800, 320))
    ->setFontPath($font)
    ->setTitle('Right chart')
    ->setSeries([['data' => [12, 18, 15, 24, 21]]])
    ->setPlotRect(440, 30, 760, 280)
    ->draw($im);
imagepng($im, __DIR__ . '/31b_two_charts_one_canvas.png');
imagedestroy($im);

/* setStrict: throw on bad data instead of silent NaN coercion.
 * Demonstrates the contract; a real production caller would catch
 * and report. */
try {
    (new FastChart\LineChart(400, 200))
    ->setFontPath($font)
    ->setDpi($dpi)
        ->setStrict(true)
        ->setSeries([['data' => [1, 2, 'oops', 4]]]);
    echo "setStrict: no throw (unexpected)\n";
} catch (\TypeError $e) {
    echo "setStrict: TypeError as expected: ", $e->getMessage(), "\n";
}

/* setBoxWidth: narrow the boxplot boxes to 40% of slot width. */
(new FastChart\BoxPlot(640, 320))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Narrow boxes (setBoxWidth(40))')
    ->setBoxWidth(40)
    ->setBoxes([
        ['label' => 'A', 'min' => 1, 'q1' => 3, 'median' => 5, 'q3' => 7, 'max' => 9],
        ['label' => 'B', 'min' => 2, 'q1' => 4, 'median' => 6, 'q3' => 8, 'max' => 10],
        ['label' => 'C', 'min' => 3, 'q1' => 5, 'median' => 7, 'q3' => 9, 'max' => 11],
    ])
    ->renderToFile(__DIR__ . '/31c_box_narrow.png');

/* setBackgroundImage: stretch a bitmap behind the chart, useful for
 * branded reports. The path is open_basedir-checked at draw time. We
 * generate a gradient PNG on the fly so the example is self-contained. */
$bg = imagecreatetruecolor(640, 320);
for ($y = 0; $y < 320; $y++) {
    $g = (int)(245 - ($y / 320) * 30);
    $c = imagecolorallocate($bg, $g, $g, $g + 5);
    imageline($bg, 0, $y, 639, $y, $c);
}
$bg_path = sys_get_temp_dir() . '/fc_bg.png';
imagepng($bg, $bg_path);
imagedestroy($bg);

(new FastChart\LineChart(640, 320))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Chart with background image')
    ->setSeries([['data' => [22, 35, 28, 41, 38, 47, 52]]])
    ->setCategoryLabels(['Mon','Tue','Wed','Thu','Fri','Sat','Sun'])
    ->setBackgroundImage($bg_path)
    ->renderToFile(__DIR__ . '/31d_bg_image.png');
@unlink($bg_path);
