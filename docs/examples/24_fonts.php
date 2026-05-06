<?php
/* Font knobs:
 *   - setFontPath / setFontSize           : global default
 *   - setTitleFont / setAxisFont / setLabelFont
 *                                         : per-role TTF override
 *   - thumbnail_mode (via setThumbnailMode) suppresses labels and
 *     scales fonts down for tile-sized previews.
 *
 * We don't ship fonts; this example uses a system DejaVu installed
 * on most Linuxes. Adjust the path for your environment if needed. */

require __DIR__ . '/_bootstrap.php';

$default = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf';
$bold    = '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf';
$serif   = '/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf';

/* Sizes default to 11px; per-role overrides are absolute in pixels.
 * Fall back to the bootstrap-resolved $font when the hard-coded
 * DejaVu path isn't installed; the global setFontPath() rejects
 * empty strings (it's not a "clear override" sentinel; that's only
 * for the per-role setters below). */
(new FastChart\LineChart(640, 360))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Per-role font override')
    ->setFontPath(file_exists($default) ? $default : $font)
    ->setFontSize(11)
    ->setTitleFont(file_exists($bold) ? $bold : '', 18)
    ->setAxisFont(file_exists($default) ? $default : '', 10)
    ->setLabelFont(file_exists($serif) ? $serif : '', 9)
    ->setSeries([['data' => [22, 35, 28, 41, 38, 47, 52]]])
    ->setCategoryLabels(['Mon','Tue','Wed','Thu','Fri','Sat','Sun'])
    ->renderToFile(__DIR__ . '/24a_font_override.png');

/* Thumbnail mode for inline tile previews. */
(new FastChart\LineChart(220, 120))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Thumb')
    ->setSeries([['data' => [22, 35, 28, 41, 38, 47, 52]]])
    ->setCategoryLabels(['M','T','W','T','F','S','S'])
    ->setThumbnailMode(true)
    ->renderToFile(__DIR__ . '/24b_thumbnail.png');
