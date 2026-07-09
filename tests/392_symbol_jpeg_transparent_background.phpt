--TEST--
Symbol JPEG flattens a transparent background onto the configured colour
--EXTENSIONS--
fastchart
gd
--SKIPIF--
<?php
if (!function_exists('imagecreatefromstring')) echo "skip gd imagecreatefromstring unavailable\n";
?>
--FILE--
<?php

/* JPEG can't carry alpha, so a transparent Symbol background must fall
 * through to the configured background colour, not a hardcoded white. */

function corner_rgb(string $jpeg): array {
    $im = imagecreatefromstring($jpeg);
    $c = imagecolorat($im, 3, 3);
    return [($c >> 16) & 0xFF, ($c >> 8) & 0xFF, $c & 0xFF];
}

$qr = (new FastChart\QrCode())
    ->setData('test')
    ->setSize(140, 140)
    ->setBackground(0x00FF00)
    ->setTransparentBackground(true);
[$r, $g, $b] = corner_rgb($qr->renderJpeg(90));
echo "transparent+green bg -> green: ", ($g > 180 && $r < 90 ? "yes" : "no ($r,$g,$b)"), "\n";

/* Default (white) background stays white when transparent. */
$qr2 = (new FastChart\QrCode())
    ->setData('test')
    ->setSize(140, 140)
    ->setTransparentBackground(true);
[$r2, $g2, $b2] = corner_rgb($qr2->renderJpeg(90));
echo "transparent+default bg -> white: ", ($r2 > 230 && $g2 > 230 && $b2 > 230 ? "yes" : "no ($r2,$g2,$b2)"), "\n";

?>
--EXPECT--
transparent+green bg -> green: yes
transparent+default bg -> white: yes
