--TEST--
Source-image load honors open_basedir at render time
--EXTENSIONS--
fastchart
gd
--SKIPIF--
<?php
$home = getenv('HOME') ?: '';
if ($home === '' || !is_dir($home) || !is_writable($home)) {
    die("skip no writable outside-basedir directory\n");
}
?>
--FILE--
<?php

/* The v0.x source-image loader did stat() -> direct fopen() (for the
 * dimension sniff) -> a second open via stream wrapper with
 * STREAM_DISABLE_OPEN_BASEDIR. The final read bypassed open_basedir
 * outright. v1.0 collapses this to a single php_stream_open_wrapper
 * without STREAM_DISABLE_OPEN_BASEDIR, so the stream wrapper enforces
 * open_basedir natively at render time.
 *
 * Test: stage an outside-basedir file while open_basedir is wide,
 * setBackgroundImage() it (passes — the setter sees the file is
 * reachable), then narrow open_basedir before renderSvg(). The
 * render-time load must refuse the outside file and emit no
 * <image> element. */

$tmp  = sys_get_temp_dir();
$home = getenv('HOME');
$pid  = getmypid();

$inside  = $tmp  . "/fc_obd_inside_{$pid}.png";
$outside = $home . "/fc_obd_outside_{$pid}.png";

$im = imagecreatetruecolor(16, 16);
imagefilledrectangle($im, 0, 0, 15, 15,
    imagecolorallocate($im, 0xFF, 0x00, 0xCC));
imagepng($im, $inside);
copy($inside, $outside);

/* Setter against the outside path while open_basedir is wide. */
$chart_outside = new FastChart\LineChart(120, 80);
$chart_outside->setBackgroundImage($outside);

/* Sanity check the inside-path leg with the same wide basedir. */
$svg_inside = (new FastChart\LineChart(120, 80))
    ->setBackgroundImage($inside)
    ->setSeries([1, 2, 3])
    ->renderSvg();
echo "wide_inside_has_image: ",
    (str_contains($svg_inside, '<image ') ? "yes" : "no"), "\n";

/* Narrow open_basedir to /tmp only — $outside is now outside the
 * allow-list. The setter already ran (and accepted); render must
 * still refuse the load. */
ini_set('open_basedir', $tmp);

$svg_outside = $chart_outside
    ->setSeries([1, 2, 3])
    ->renderSvg();
echo "narrow_outside_has_image: ",
    (str_contains($svg_outside, '<image ') ? "yes" : "no"), "\n";

/* The inside path is still in basedir after narrowing — its render
 * should still embed the image. Confirms the loader didn't go
 * fail-closed on all paths. */
$svg_inside2 = (new FastChart\LineChart(120, 80))
    ->setBackgroundImage($inside)
    ->setSeries([1, 2, 3])
    ->renderSvg();
echo "narrow_inside_has_image: ",
    (str_contains($svg_inside2, '<image ') ? "yes" : "no"), "\n";

@unlink($inside);
echo "ok\n";
?>
--CLEAN--
<?php
$home = getenv('HOME');
$pid  = getmypid();
@unlink($home . "/fc_obd_outside_{$pid}.png");
?>
--EXPECT--
wide_inside_has_image: yes
narrow_outside_has_image: no
narrow_inside_has_image: yes
ok
