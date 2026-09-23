<?php
/* Every way to get pixels or vector markup out of a chart:
 *   - renderToFile($path)         : file, format inferred from extension
 *                                   (.svg | .png | .jpg | .webp)
 *   - renderSvg()                 : full SVG document (vector)
 *   - drawSvgFragment()           : <g>...</g> for stitching into an
 *                                   outer SVG document
 *   - renderPng()                 : PNG bytes (libpng, lossless)
 *   - renderJpeg($quality = null) : JPEG bytes (libjpeg-turbo); null
 *                                   means "use setJpegQuality()", default 88
 *   - renderWebp($quality = 90)   : WebP bytes (libwebp)
 *
 * Use the bytes-returning helpers for HTTP responses, base64 data
 * URIs, or hashing. */

require __DIR__ . '/_bootstrap.php';

$line = (new FastChart\LineChart(400, 200))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Output formats demo')
    ->setSeries([['data' => [10, 20, 15, 25, 22, 30]]])
    ->setCategoryLabels(['Mon','Tue','Wed','Thu','Fri','Sat']);

/* Direct-to-file: format from extension. */
$line->renderToFile(__DIR__ . '/20a_renderToFile.png');

/* Print each result's header bytes as a format check. */
$svg  = $line->renderSvg();
$png  = $line->renderPng();
$jpg  = $line->renderJpeg(85);
$webp = $line->renderWebp(80);

printf("svg:  %d bytes, prolog=%s\n", strlen($svg),
       substr($svg, 0, 38));
printf("png:  %d bytes, magic=%s\n",  strlen($png),  bin2hex(substr($png, 0, 4)));
printf("jpeg: %d bytes, magic=%s\n",  strlen($jpg),  bin2hex(substr($jpg, 0, 4)));
printf("webp: %d bytes, magic=%s\n",  strlen($webp), bin2hex(substr($webp, 0, 4)));

/* Save the same PNG bytes to disk as a sanity check. */
file_put_contents(__DIR__ . '/20b_renderPng.png', $png);
