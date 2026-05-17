--TEST--
PolarChart::addVectors(): non-finite doubles rejected (regression: render-time int-cast UB)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Render-time UB regression: render maps r * v->radius / rmax through
 * (int)(r0 * cos(a0)) at fastchart_polar.c:239-246. A NaN or +/-Inf
 * radius/angle reaches the float-to-int conversion, which is undefined
 * behavior per C / Annex F. Every other polar setter rejects non-finite
 * at the boundary (setSeries via fastchart_zval_to_double; setMaxRadius
 * via fastchart_reject_non_finite). addVectors must do the same. */

$base = function() {
    return (new FastChart\PolarChart())
        ->setSize(200, 200)
        ->setSeries([[0, 5]])
        ->setMaxRadius(10);
};

foreach ([
    'NAN angle'      => ['angle' => NAN, 'radius' => 0, 'angle_to' => 0, 'radius_to' => 3],
    '+INF radius'    => ['angle' => 0, 'radius' => INF, 'angle_to' => 0, 'radius_to' => 3],
    '-INF angle_to'  => ['angle' => 0, 'radius' => 0, 'angle_to' => -INF, 'radius_to' => 3],
    'NAN radius_to'  => ['angle' => 0, 'radius' => 0, 'angle_to' => 0, 'radius_to' => NAN],
] as $label => $entry) {
    $c = $base();
    try {
        $c->addVectors([$entry]);
        echo "BUG: $label accepted\n";
    } catch (\TypeError|\ValueError $e) {
        echo "$label rejected: ok\n";
    }
}

/* Atomicity: a valid entry followed by a non-finite entry must not
 * leak the valid prefix into self->vectors (matches existing throw
 * discipline in addVectors). */
$c = $base();
$pre = substr_count($c->renderSvg(), '<line');
try {
    $c->addVectors([
        ['angle' => 30, 'radius' => 0, 'angle_to' => 30, 'radius_to' => 4],  /* valid */
        ['angle' => 60, 'radius' => 0, 'angle_to' => 60, 'radius_to' => NAN], /* bad */
    ]);
    echo "BUG: mixed batch accepted\n";
} catch (\TypeError|\ValueError $e) {
    echo "mixed batch rejected: ok\n";
}
$post = substr_count($c->renderSvg(), '<line');
echo "no partial commit: ", $pre === $post ? 'ok' : "BAD ($pre vs $post)", "\n";

/* Sanity: finite values still work. */
$c = $base();
$c->addVectors([['angle' => 0, 'radius' => 0, 'angle_to' => 0, 'radius_to' => 5]]);
echo "finite value still works: ", strpos($c->renderSvg(), '<line') !== false ? 'ok' : 'BAD', "\n";
?>
--EXPECT--
NAN angle rejected: ok
+INF radius rejected: ok
-INF angle_to rejected: ok
NAN radius_to rejected: ok
mixed batch rejected: ok
no partial commit: ok
finite value still works: ok
