--TEST--
PieChart: variable-radius (rose) pie via the per-slice "radius" metric
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* A positive "radius" on any slice switches PieChart to a
 * variable-radius (rose) pie: each slice keeps its value-proportional
 * angle but its outer radius scales with the radius metric. With equal
 * values the angles match a plain pie, so the rose silhouette differs
 * only because the radii differ. Re-calling setSlices() without any
 * radius key clears the mode. */

function valid(string $svg): bool {
    return strlen($svg) > 100 &&
        simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

$varied = [
    ['label' => 'A', 'value' => 20, 'radius' => 30],
    ['label' => 'B', 'value' => 20, 'radius' => 60],
    ['label' => 'C', 'value' => 20, 'radius' => 90],
    ['label' => 'D', 'value' => 20, 'radius' => 50],
];
/* Same values and angles, but every radius equal -> every slice draws
 * at full radius, i.e. visually a plain pie. */
$uniform = [
    ['label' => 'A', 'value' => 20, 'radius' => 50],
    ['label' => 'B', 'value' => 20, 'radius' => 50],
    ['label' => 'C', 'value' => 20, 'radius' => 50],
    ['label' => 'D', 'value' => 20, 'radius' => 50],
];
$plain = [
    ['label' => 'A', 'value' => 20],
    ['label' => 'B', 'value' => 20],
    ['label' => 'C', 'value' => 20],
    ['label' => 'D', 'value' => 20],
];

$rose      = (new FastChart\PieChart(420, 360))->setSlices($varied)->renderSvg();
$uni       = (new FastChart\PieChart(420, 360))->setSlices($uniform)->renderSvg();
$plainPie  = (new FastChart\PieChart(420, 360))->setSlices($plain)->renderSvg();

echo "rose_valid: ", valid($rose) ? "yes" : "no", "\n";
echo "rose_finite: ", (strpos($rose, '-2147483648') === false) ? "yes" : "no", "\n";
echo "rose_differs_from_uniform: ", ($rose !== $uni) ? "yes" : "no", "\n";
echo "uniform_equals_plain_geometry: ", ($uni === $plainPie) ? "yes" : "no", "\n";

/* Re-setting slices without a radius key resets the mode: the second
 * render matches a chart that never had radii. */
$chart = new FastChart\PieChart(420, 360);
$chart->setSlices($varied);
$chart->setSlices($plain);
echo "reset_clears_mode: ", ($chart->renderSvg() === $plainPie) ? "yes" : "no", "\n";

/* The largest-radius slice drives normalization. If it is folded into
 * the "Other" bucket by setOtherThreshold, the surviving slices must
 * still scale up so the largest survivor reaches the full radius -- the
 * max is recomputed over the drawn slices, not the setter-time set.
 * Extract the largest wedge radius (the "A<r>,<r>" arc command). */
function max_arc_radius(string $svg): int {
    preg_match_all('/\bA(\d+),\d+/', $svg, $m);
    return $m[1] ? max(array_map('intval', $m[1])) : 0;
}

/* Reference full radius: a plain pie at the same canvas (no radius keys)
 * draws every slice at the full radius. */
$fullRef = max_arc_radius(
    (new FastChart\PieChart(420, 360))->setSlices([['label'=>'A','value'=>30], ['label'=>'C','value'=>30]])->renderSvg()
);

/* B carries the largest radius but a tiny value, so a 10% threshold
 * folds it into "Other". A and C (radius 40 each) are the survivors;
 * with the fix the larger of them reaches the full radius. */
$folded = (new FastChart\PieChart(420, 360))
    ->setSlices([
        ['label'=>'A','value'=>30,'radius'=>40],
        ['label'=>'B','value'=>2, 'radius'=>80],
        ['label'=>'C','value'=>30,'radius'=>40],
    ])
    ->setOtherThreshold(10)
    ->renderSvg();
echo "survivor_reaches_full_radius: ", (max_arc_radius($folded) === $fullRef && $fullRef > 0) ? "yes" : "no", "\n";

echo "ok\n";
?>
--EXPECT--
rose_valid: yes
rose_finite: yes
rose_differs_from_uniform: yes
uniform_equals_plain_geometry: yes
reset_clears_mode: yes
survivor_reaches_full_radius: yes
ok
