--TEST--
WEBP_LOSSLESS round-trips bit-exact against the PNG render
--EXTENSIONS--
fastchart
gd
--SKIPIF--
<?php
if (!function_exists('imagecreatefromwebp')) die('skip gd built without webp support');
?>
--FILE--
<?php

/* Both render paths rasterize the same SVG into the same RGBA
 * buffer; PNG is lossless by construction, so a truly lossless WebP
 * must decode to identical pixels. The saturated-red foreground is
 * load-bearing: black-on-white survives YUV420 (neutral chroma
 * everywhere, luma roundtrips exactly for 0/255), but colored module
 * edges have non-neutral chroma that 4:2:0 decimation bleeds —
 * pre-fix (RGBA imported via YUV420) this comparison reports
 * thousands of differing pixels. */

$q = (new FastChart\QrCode())
    ->setData('https://example.org/fastchart-lossless-roundtrip')
    ->setForeground(0xCC0022)
    ->setWebpMode(FastChart\Chart::WEBP_LOSSLESS);

$png  = $q->renderPng();
$webp = $q->renderWebp();

/* Lossless container: chunk fourCC after the WEBP tag is VP8L. */
var_dump(substr($webp, 12, 4));

$ip = imagecreatefromstring($png);
$iw = imagecreatefromwebp('data://application/octet-stream;base64,'
    . base64_encode($webp));

$w = imagesx($ip);
$h = imagesy($ip);
var_dump($w === imagesx($iw) && $h === imagesy($iw));

$diff = 0;
for ($y = 0; $y < $h; $y++) {
    for ($x = 0; $x < $w; $x++) {
        if (imagecolorat($ip, $x, $y) !== imagecolorat($iw, $x, $y)) {
            $diff++;
        }
    }
}
var_dump($diff);

?>
--EXPECT--
string(4) "VP8L"
bool(true)
int(0)
