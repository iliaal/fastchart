--TEST--
Funnel STYLE_CONE: adjacent bands tile without white gaps
--EXTENSIONS--
fastchart
gd
--SKIPIF--
<?php
if (!function_exists('imagecreatefromstring')) die('skip gd imagecreatefromstring required');
?>
--FILE--
<?php

/* The cone's front-facing band edges are ellipse arcs that dip below
 * each boundary. Regression: the dip depth was derived from each band's
 * AVERAGE half-width, so a shared boundary got two different depths and
 * left a white crescent between bands (widest at the centre line). The
 * fix depths each arc from the half-width AT that boundary, so a band's
 * bottom arc and the next band's top arc coincide.
 *
 * Probe the centre column top-to-bottom: with the bands tiling, the
 * cone is one long unbroken run of non-background pixels. A gap would
 * split that run into per-band pieces (the tallest band here is ~180px
 * of a 460px canvas), so a single run spanning most of the height
 * proves the seam closed. */

$png = (new FastChart\Funnel(420, 460))
    ->setStyle(FastChart\Funnel::STYLE_CONE)
    ->setStages([
        ['value' => 1000], ['value' => 640], ['value' => 380], ['value' => 160],
    ])
    ->renderPng();

$im = imagecreatefromstring($png);
if ($im === false) { echo "decode_failed\n"; exit; }
$w = imagesx($im);
$h = imagesy($im);
$cx = (int)($w / 2);

$best = 0;
$cur = 0;
for ($y = 0; $y < $h; $y++) {
    $rgb = imagecolorat($im, $cx, $y);
    $r = ($rgb >> 16) & 0xFF;
    $g = ($rgb >> 8) & 0xFF;
    $b = $rgb & 0xFF;
    $background = ($r > 240 && $g > 240 && $b > 240);
    if (!$background) {
        $cur++;
        if ($cur > $best) $best = $cur;
    } else {
        $cur = 0;
    }
}

echo "size_ok: ", ($w === 420 && $h === 460) ? "yes" : "no", "\n";
echo "cone_unbroken: ", ($best > 350) ? "yes" : "no", "\n";
echo "ok\n";
?>
--EXPECT--
size_ok: yes
cone_unbroken: yes
ok
