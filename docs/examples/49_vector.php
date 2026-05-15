<?php
/* VectorChart: arrow-on-grid vector field. Each datum draws an
 * arrow at (x,y) pointing in (dx,dy) direction; arrow length
 * tracks magnitude relative to the field's max. */

require __DIR__ . '/_bootstrap.php';

/* Synthetic 2D rotational field: arrows swirl around (5, 5)
 * proportional to distance from center — a classic rotational
 * field plot. */
$vecs = [];
$cx = 5; $cy = 5;
for ($x = 0; $x <= 10; $x++) {
    for ($y = 0; $y <= 10; $y++) {
        $dx = -($y - $cy) * 0.3;
        $dy =  ($x - $cx) * 0.3;
        $vecs[] = ['x' => $x, 'y' => $y, 'dx' => $dx, 'dy' => $dy];
    }
}

(new FastChart\VectorChart(560, 520))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Rotational vector field')
    ->setVectors($vecs)
    ->setColorRamp(0xDDE7FF, 0x1E3A8A)
    ->renderToFile(__DIR__ . '/49_vector.png');
