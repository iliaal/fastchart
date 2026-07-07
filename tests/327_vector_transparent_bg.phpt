--TEST--
VectorChart honors setTransparentBackground() via the shared canvas-bg helper
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

/* VectorChart painted an unconditional opaque full-canvas rect instead
 * of calling fastchart_paint_canvas_bg(), so setTransparentBackground
 * (and setPlotRect / bg-image compositing) were ignored. A transparent
 * render must leave the canvas corner fully transparent (gd alpha 127). */

function corner_alpha(string $png): int {
    $im = imagecreatefromstring($png);
    return (imagecolorat($im, 2, 2) >> 24) & 0x7F;
}

$vecs = [
    ['x' => 0, 'y' => 0, 'dx' => 0.3, 'dy' => 0.3],
    ['x' => 1, 'y' => 1, 'dx' => 0.5, 'dy' => 0.2],
];

$transparent = (new FastChart\VectorChart(200, 200))
    ->setVectors($vecs)
    ->setTransparentBackground(true)
    ->renderPng();
echo "transparent corner: ", corner_alpha($transparent) === 127 ? "yes" : "no", "\n";

/* Opaque control: without the flag the corner is fully opaque. */
$opaque = (new FastChart\VectorChart(200, 200))
    ->setVectors($vecs)
    ->renderPng();
echo "opaque control: ", corner_alpha($opaque) === 0 ? "yes" : "no", "\n";

?>
--EXPECT--
transparent corner: yes
opaque control: yes
