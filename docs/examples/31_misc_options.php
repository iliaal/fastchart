<?php
/* Catch-all for the remaining utility setters in v1.0:
 *   - FastChart\Chart::version()       : extension version string (static)
 *   - setSize(w, h)                    : change canvas size after construction
 *     (constructor accepts the same args; setter is for already-built objects)
 *   - setPlotRect(x0, y0, x1, y1)      : pin the plot rectangle to fixed
 *     pixel coordinates instead of letting the layout helper pick. Useful
 *     when stitching multiple chart fragments into one outer SVG document.
 *   - setStrict(true)                  : reject non-numeric Line/Area/Bar
 *     setSeries cells with a TypeError instead of silently coercing to NaN
 *   - setBoxWidth(percent)             : boxplot box width as a percent of
 *     the per-category slot
 *   - setBackgroundImage(path)         : overlay a background image
 *
 * The v1.0 cleanup removed draw($canvas) / renderGif / renderAvif;
 * the two-charts-on-one-canvas pattern now goes through
 * drawSvgFragment() + outer-SVG composition (see 31b below).
 *
 * ext/gd is no longer a fastchart runtime dependency. This example
 * only uses ext/gd to build a synthetic gradient background image
 * for setBackgroundImage; if gd is not loaded, that section skips. */

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

/* setPlotRect + drawSvgFragment: stack multiple charts in one outer
 * SVG document. setPlotRect pins each chart's plot rectangle to a
 * known position; the outer <svg> places each fragment at the same
 * world-coordinate origin so the plot rects line up exactly. */
$W = 800; $H = 320;
$left_g = (new FastChart\LineChart($W, $H))
    ->setFontPath($font)
    ->setTitle('Left chart')
    ->setSeries([['data' => [22, 35, 28, 41, 38]]])
    ->setPlotRect(60, 30, 380, 280)
    ->drawSvgFragment();
$right_g = (new FastChart\BarChart($W, $H))
    ->setFontPath($font)
    ->setTitle('Right chart')
    ->setSeries([['data' => [12, 18, 15, 24, 21]]])
    ->setPlotRect(440, 30, 760, 280)
    ->drawSvgFragment();

$svg  = '<?xml version="1.0" encoding="UTF-8"?>' . "\n";
$svg .= '<svg xmlns="http://www.w3.org/2000/svg" width="' . $W . '" height="' . $H
      . '" viewBox="0 0 ' . $W . ' ' . $H . '">';
$svg .= '<rect width="100%" height="100%" fill="#F5F5F5"/>';
$svg .= $left_g;
$svg .= $right_g;
$svg .= '</svg>';
file_put_contents(__DIR__ . '/31b_two_charts_one_canvas.svg', $svg);

/* setStrict: throw on bad data instead of silent NaN coercion. */
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
 * generate a gradient PNG on the fly via ext/gd when available. */
if (function_exists('imagecreatetruecolor')) {
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
} else {
    echo "setBackgroundImage: skipped (ext/gd not loaded — needed to "
       . "build the synthetic gradient image for the demo)\n";
}
