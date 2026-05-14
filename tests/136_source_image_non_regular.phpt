--TEST--
Source-image loader rejects non-regular files (S_ISREG defense-in-depth)
--EXTENSIONS--
fastchart
gd
--SKIPIF--
<?php
if (!is_readable('/proc/self/cmdline')) {
    die("skip /proc not available\n");
}
?>
--FILE--
<?php

/* The stream wrapper happily opens any readable inode. Without an
 * S_ISREG check after open, a render fed /proc/self/cmdline (or a
 * directory, FIFO, character device) would read either nothing or
 * unbounded data before the MIME sniff catches it. The defense-in-
 * depth check rejects non-regular files up-front so the rest of
 * the pipeline only sees disk file bytes. */

/* Directory — open succeeds on Linux, would emit zero bytes on read. */
$svg_dir = (new FastChart\LineChart(120, 80))
    ->setBackgroundImage(sys_get_temp_dir())
    ->setSeries([1, 2, 3])
    ->renderSvg();
echo "directory_has_image: ",
    (strpos($svg_dir, '<image ') !== false ? "yes" : "no"), "\n";

/* /proc entry — open succeeds; content is a NUL-separated argv blob,
 * not a PNG/JPEG; MIME sniff would catch it but S_ISREG catches it
 * earlier. */
$svg_proc = (new FastChart\LineChart(120, 80))
    ->setBackgroundImage('/proc/self/cmdline')
    ->setSeries([1, 2, 3])
    ->renderSvg();
echo "proc_has_image: ",
    (strpos($svg_proc, '<image ') !== false ? "yes" : "no"), "\n";

/* Regular file still works — sanity check the check isn't fail-closed. */
$tmp = tempnam(sys_get_temp_dir(), 'fc_isreg_');
$im = imagecreatetruecolor(8, 8);
imagepng($im, $tmp);
$svg_ok = (new FastChart\LineChart(120, 80))
    ->setBackgroundImage($tmp)
    ->setSeries([1, 2, 3])
    ->renderSvg();
@unlink($tmp);
echo "regular_has_image: ",
    (strpos($svg_ok, '<image ') !== false ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
directory_has_image: no
proc_has_image: no
regular_has_image: yes
ok
