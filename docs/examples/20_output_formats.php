<?php
/* Every way to get pixels out of a chart:
 *   - renderToFile($path)         : file, format inferred from extension
 *   - renderPng()                 : PNG bytes
 *   - renderJpeg($quality = 85)   : JPEG bytes
 *   - renderWebp($quality = 80)   : WebP bytes (libgd build-time toggle)
 *   - renderAvif($quality = 50)   : AVIF bytes (libgd 2.4+)
 *   - renderGif()                 : GIF bytes (paletted)
 *   - draw($gd_image)             : draw onto a caller-owned canvas
 *
 * The bytes-returning helpers skip the encode-to-disk roundtrip and
 * are convenient for HTTP responses, base64 data URIs, or hashing. */

require __DIR__ . '/_bootstrap.php';

$line = (new FastChart\LineChart(400, 200))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Output formats demo')
    ->setSeries([['data' => [10, 20, 15, 25, 22, 30]]])
    ->setCategoryLabels(['Mon','Tue','Wed','Thu','Fri','Sat']);

/* Direct-to-file: format from extension. */
$line->renderToFile(__DIR__ . '/20a_renderToFile.png');

/* Bytes-returning helpers. Print a tiny header summary so the
 * example doubles as a shape check. */
$png  = $line->renderPng();
$jpg  = $line->renderJpeg(85);
$webp = $line->renderWebp(80);
$gif  = $line->renderGif();

printf("png:  %d bytes, magic=%s\n",  strlen($png),  bin2hex(substr($png, 0, 4)));
printf("jpeg: %d bytes, magic=%s\n",  strlen($jpg),  bin2hex(substr($jpg, 0, 4)));
printf("webp: %d bytes, magic=%s\n",  strlen($webp), bin2hex(substr($webp, 0, 4)));
printf("gif:  %d bytes, magic=%s\n",  strlen($gif),  bin2hex(substr($gif, 0, 4)));

/* AVIF is libgd >= 2.4 only. Wrap in a try/catch so the example
 * still runs on older libgd. */
try {
    $avif = $line->renderAvif(50);
    printf("avif: %d bytes\n", strlen($avif));
} catch (\Throwable $e) {
    echo "avif: not supported by this libgd build\n";
}

/* Save the same PNG bytes to disk as a sanity check. */
file_put_contents(__DIR__ . '/20b_renderPng.png', $png);
