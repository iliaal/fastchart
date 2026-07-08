--TEST--
PolarChart::addVectors caps cumulative vectors at FASTCHART_MAX_VECTORS (regression: DoS)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: addVectors was the only additive array setter missing the
 * FASTCHART_MAX_VECTORS (4096) ceiling its siblings enforce — one huge
 * array (emalloc(incoming * sizeof)) or repeated calls grew self->vectors
 * without limit (memory-exhaustion DoS). Each vector renders as ~3 <line>
 * elements (shaft + two arrowhead strokes), so the drawn-vector count is
 * (line_count - spokes_and_rings) / 3. Adding more than 4096 must reject. */

function vectors_drawn($n) {
    $c = (new FastChart\PolarChart())->setSize(300, 300)->setSeries([[0, 5]])->setMaxRadius(10);
    $base = substr_count($c->renderSvg(), '<line');   /* spokes/rings only, no vectors */
    $b = [];
    for ($i = 0; $i < $n; $i++) {
        $b[] = ['angle' => 0, 'radius' => 0, 'angle_to' => $i * 0.01, 'radius_to' => 3];
    }
    $c->addVectors($b);
    return intdiv(substr_count($c->renderSvg(), '<line') - $base, 3);
}

echo "n=100:   ", vectors_drawn(100), "\n";
echo "n=4096:  ", vectors_drawn(4096), "\n";

try {
    vectors_drawn(4097);
    echo "n=4097:  no\n";
} catch (ValueError $e) {
    echo "n=4097:  yes\n";
}

$c = (new FastChart\PolarChart())->setSeries([[0, 5]])->setMaxRadius(10);
$b = [];
for ($i = 0; $i < 4096; $i++) {
    $b[] = ['angle' => 0, 'radius' => 0, 'angle_to' => $i * 0.01, 'radius_to' => 3];
}
$c->addVectors($b);
try {
    $c->addVectors([['angle' => 0, 'radius' => 0, 'angle_to' => 0, 'radius_to' => 3]]);
    echo "cumulative_extra: no\n";
} catch (ValueError $e) {
    echo "cumulative_extra: yes\n";
}
echo "done\n";
?>
--EXPECT--
n=100:   100
n=4096:  4096
n=4097:  yes
cumulative_extra: yes
done
