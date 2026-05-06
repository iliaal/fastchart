--TEST--
setBackgroundImage / addIconAt skip source files larger than 8 MiB
--EXTENSIONS--
fastchart
gd
--FILE--
<?php
/* Forge an oversized PNG-ish file (the loader rejects it on size
 * before reading the magic bytes, so the contents don't matter).
 * The render must still produce a chart — the cap is a silent skip,
 * not a hard error. Then verify a small image is still accepted and
 * actually changes the output, so we know the cap path didn't
 * neuter the legitimate-asset path. */
$tmp_big   = tempnam(sys_get_temp_dir(), 'fc_big_')   . '.png';
$tmp_small = tempnam(sys_get_temp_dir(), 'fc_small_') . '.png';

/* 9 MiB of zeros — over the 8 MiB cap. */
$fh = fopen($tmp_big, 'wb');
fwrite($fh, str_repeat("\0", 9 * 1024 * 1024));
fclose($fh);

/* Small valid 32x32 PNG via ext/gd. */
$im = imagecreatetruecolor(32, 32);
imagefill($im, 0, 0, imagecolorallocate($im, 0xCC, 0x33, 0x66));
imagepng($im, $tmp_small);
imagedestroy($im);

/* Baseline render (no background). */
$base = (new FastChart\LineChart(160, 100))
    ->setSeries([['data' => [1, 2, 3]]])
    ->renderPng();

/* With the oversized background — must succeed AND must produce
 * the same bytes as the baseline (loader skipped it). */
$with_big = (new FastChart\LineChart(160, 100))
    ->setSeries([['data' => [1, 2, 3]]])
    ->setBackgroundImage($tmp_big)
    ->renderPng();
echo "big_skipped: ", ($with_big === $base ? "yes" : "no"), "\n";

/* With a small valid background — must succeed AND must differ
 * from the baseline (loader composited it). */
$with_small = (new FastChart\LineChart(160, 100))
    ->setSeries([['data' => [1, 2, 3]]])
    ->setBackgroundImage($tmp_small)
    ->renderPng();
echo "small_used:  ", ($with_small !== $base ? "yes" : "no"), "\n";

@unlink($tmp_big);
@unlink($tmp_small);
?>
--EXPECT--
big_skipped: yes
small_used:  yes
