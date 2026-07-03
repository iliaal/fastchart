--TEST--
SurfaceChart and SerpentineTimeline honor setTransparentBackground()
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

/* Regression (fnd_822406a5, fnd_e33bd054): SurfaceChart and
 * SerpentineTimeline hand-rolled their canvas fill with an unconditional
 * full-canvas rect, so setTransparentBackground (and setPlotRect / bg
 * image compositing) were ignored. Both now route through the shared
 * fastchart_paint_canvas_bg helper. A transparent render must leave the
 * canvas corner fully transparent (gd alpha 127). */

function corner_alpha(string $png): int {
    /* No imagedestroy(): it is a no-op since PHP 8.0 and deprecated in 8.5,
     * where the notice would pollute the asserted output. GC frees $im. */
    $im = imagecreatefromstring($png);
    return (imagecolorat($im, 2, 2) >> 24) & 0x7F;
}

$sf = (new FastChart\SurfaceChart(200, 200))
    ->setGrid([[1, 2, 3], [4, 5, 6], [7, 8, 9]])
    ->setTransparentBackground(true)
    ->renderPng();
echo "surface transparent: ", corner_alpha($sf) === 127 ? "yes" : "no", "\n";

$sp = (new FastChart\SerpentineTimeline(300, 150))
    ->setEvents([['label' => 'A', 'date' => '2020'], ['label' => 'B', 'date' => '2021']])
    ->setTransparentBackground(true)
    ->renderPng();
echo "serpentine transparent: ", corner_alpha($sp) === 127 ? "yes" : "no", "\n";

/* Opaque control: without the flag the corner is fully opaque. */
$op = (new FastChart\SurfaceChart(200, 200))
    ->setGrid([[1, 2, 3], [4, 5, 6], [7, 8, 9]])
    ->renderPng();
echo "opaque control: ", corner_alpha($op) === 0 ? "yes" : "no", "\n";

?>
--EXPECT--
surface transparent: yes
serpentine transparent: yes
opaque control: yes
