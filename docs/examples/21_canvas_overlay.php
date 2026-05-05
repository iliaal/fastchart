<?php
/* Caller-owned canvas: draw() returns the same \GdImage you passed in,
 * so you can layer additional graphics on top without ever leaving the
 * gd surface. Useful for watermarks, footers, logos, highlight rings,
 * compositing multiple charts on one canvas, post-process filters
 * like imagefilter() or imagecopyresized(), etc.
 *
 * This example builds a stock chart, then overlays:
 *   - a translucent "DRAFT" watermark across the body
 *   - a footer credit line at the bottom
 *   - a colored ring highlighting a specific data point
 * All using stock ext/gd primitives on the same canvas the chart
 * just rendered into. */

require __DIR__ . '/_bootstrap.php';

mt_srand(3);
$rows = [];
$base = 1700000000;
$price = 100.0;
for ($i = 0; $i < 60; $i++) {
    $delta = (mt_rand(-100, 100) / 100.0) * 1.4;
    $open  = $price;
    $close = $price + $delta;
    $high  = max($open, $close) + mt_rand(0, 60) / 100.0;
    $low   = min($open, $close) - mt_rand(0, 60) / 100.0;
    $rows[] = [$base + $i * 86400, $open, $high, $low, $close, 1000];
    $price  = $close;
}

$im = imagecreatetruecolor(720, 420);

/* Step 1: render the chart into our caller-owned canvas. */
(new FastChart\StockChart(720, 420))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('ACME — internal preview')
    ->setOhlcv($rows)
    ->addMovingAverage(20)
    ->draw($im);

/* Step 2: overlay a translucent "DRAFT" watermark using gd's
 * built-in 5x7 bitmap font, scaled up via repeated imagestring. */
$watermark = imagecolorallocatealpha($im, 200, 30, 30, 90);
$cx = imagesx($im) / 2;
$cy = imagesy($im) / 2;
imagestring($im, 5, (int)$cx - 30, (int)$cy - 8, 'DRAFT', $watermark);

/* Step 3: add a footer credit line. */
$footer = imagecolorallocate($im, 100, 100, 100);
imagestring($im, 2, 8, imagesy($im) - 16,
            'Generated ' . date('Y-m-d') . ' — confidential', $footer);

/* Step 4: draw a highlight ring around the most recent close.
 * The chart object doesn't expose pixel positions, but for an
 * overlay you control the canvas size and can derive coordinates
 * from your own data shape — here approximated as the last
 * candle's relative position. */
$ring = imagecolorallocate($im, 255, 220, 0);
imageellipse($im, imagesx($im) - 60, 200, 24, 24, $ring);
imageellipse($im, imagesx($im) - 60, 200, 26, 26, $ring);

imagepng($im, __DIR__ . '/21_canvas_overlay.png');
imagedestroy($im);
