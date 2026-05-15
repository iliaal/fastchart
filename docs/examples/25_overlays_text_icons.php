<?php
/* Three annotation-family helpers in one chart:
 *   - addOverlaySeries(type, values, opts)   : extra polyline / area
 *     traced on top of the main chart, with its own color/thickness
 *   - addTextAnnotation(text, x, y, color?)  : free-floating text at
 *     a chart-local coordinate (data X categorical index, data Y)
 *   - addIconAt(x, y, path, w?, h?)          : overlay an external
 *     image at data coordinates. Useful for inline event markers,
 *     logos, or status icons.
 *
 * addIconAt needs an image file on disk. fastchart's loader accepts
 * PNG and JPEG; plutosvg's data-URI consumer in the SVG backend has
 * the same restriction. We generate a tiny star icon via ext/gd if
 * it's loaded, otherwise we fall back to a plain marker and skip the
 * icon overlay. ext/gd is no longer a fastchart runtime dependency,
 * so this example is portable to gd-less PHP builds. */

require __DIR__ . '/_bootstrap.php';

$icon_path = null;
if (function_exists('imagecreatetruecolor')) {
    $icon = imagecreatetruecolor(20, 20);
    imagealphablending($icon, false);
    imagesavealpha($icon, true);
    $bg = imagecolorallocatealpha($icon, 0, 0, 0, 127);
    imagefilledrectangle($icon, 0, 0, 19, 19, $bg);
    $marker = imagecolorallocate($icon, 255, 200, 0);
    imagefilledpolygon($icon, [10, 0, 13, 7, 19, 7, 14, 12, 16, 19,
                               10, 15, 4, 19, 6, 12, 1, 7, 7, 7], $marker);
    $icon_path = sys_get_temp_dir() . '/fc_star.png';
    imagepng($icon, $icon_path);
    imagedestroy($icon);
}

$chart = (new FastChart\LineChart(720, 380))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Daily traffic with overlay + text + icon annotations')
    ->setSeries([
        ['label' => 'Visitors',
         'data'  => [4200, 4500, 4100, 5200, 6800, 7100, 5400, 4900, 6200, 7800]],
    ])
    ->setCategoryLabels(['D1','D2','D3','D4','D5','D6','D7','D8','D9','D10'])
    ->addOverlaySeries('line',
        [4500, 4500, 4500, 4500, 4500, 4500, 4500, 4500, 4500, 4500],
        ['color' => 0x9D9D9D, 'thickness' => 1])
    ->addTextAnnotation('campaign launch', 3, 7000, 0xE34A6F)
    ->setMarkerStyle(FastChart\Chart::MARKER_CIRCLE);

if ($icon_path !== null) {
    $chart->addIconAt(3, 5200, $icon_path, 20, 20);
}

$chart->renderToFile(__DIR__ . '/25_overlays_text_icons.png');

if ($icon_path !== null) {
    @unlink($icon_path);
}
