--TEST--
PieChart: semi-circle sweep window and nested-donut rings
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* setStartAngle/setEndAngle narrow the sweep to a partial pie; the
 * default full pie and the half pie must both be valid finite SVG.
 * setRings() replaces the flat pie with concentric bands, one slice set
 * per ring, drawn outermost-first with thin separator circles. */

function valid(string $svg): bool {
    return strlen($svg) > 100 &&
        simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

$slices = ['A' => 3, 'B' => 2, 'C' => 5];

$full = (new FastChart\PieChart(400, 400))->setSlices($slices)->renderSvg();
$half = (new FastChart\PieChart(400, 400))->setSlices($slices)
    ->setStartAngle(0)->setEndAngle(180)->renderSvg();

echo "full_valid: ", valid($full) ? "yes" : "no", "\n";
echo "half_valid: ", valid($half) ? "yes" : "no", "\n";
echo "half_clean: ", (strpos($half, '-2147483648') === false ? "yes" : "no"), "\n";

/* A degenerate window (end <= start) falls back to the full circle. */
$bad = (new FastChart\PieChart(400, 400))->setSlices($slices)
    ->setStartAngle(200)->setEndAngle(100)->renderSvg();
echo "degenerate_valid: ", valid($bad) ? "yes" : "no", "\n";

/* Nested donut: three rings. Each ring contributes <path> wedges, so the
 * nested render has more wedge paths than a single-ring pie of one set. */
$nested = (new FastChart\PieChart(500, 500))
    ->setRings([
        ['A' => 1, 'B' => 1],
        ['X' => 2, 'Y' => 1, 'Z' => 1],
        ['M' => 3, 'N' => 2],
    ])
    ->setDonutHoleRatio(0.4)
    ->renderSvg();

$oneRing = (new FastChart\PieChart(500, 500))
    ->setRings([['A' => 1, 'B' => 1]])
    ->renderSvg();

echo "nested_valid: ", valid($nested) ? "yes" : "no", "\n";
echo "nested_clean: ", (strpos($nested, '-2147483648') === false ? "yes" : "no"), "\n";
echo "nested_has_more_wedges: ",
    (substr_count($nested, '<path') > substr_count($oneRing, '<path') ? "yes" : "no"), "\n";

/* Rings supersede flat slices: setRings with empty rows leaves nothing
 * to draw but must not crash and falls through to the slice path. */
$flat = (new FastChart\PieChart(400, 400))->setSlices($slices)
    ->setRings([])->renderSvg();
echo "empty_rings_uses_slices: ", valid($flat) ? "yes" : "no", "\n";

/* Clone isolation: ring data is the only new owned-pointer surface. A
 * clone must deep-copy it and still render after the original is freed
 * (catches a shared-pointer double-free / UAF under ASAN). */
$mk = fn() => (new FastChart\PieChart(500, 500))
    ->setRings([['A' => 1, 'B' => 1], ['X' => 2, 'Y' => 1]])
    ->setDonutHoleRatio(0.4);
$ref  = $mk()->renderSvg();
$orig = $mk();
$copy = clone $orig;
unset($orig);
$out = $copy->renderSvg();
echo "ring_clone_isolated: ", ($out === $ref && valid($out) ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
full_valid: yes
half_valid: yes
half_clean: yes
degenerate_valid: yes
nested_valid: yes
nested_clean: yes
nested_has_more_wedges: yes
empty_rings_uses_slices: yes
ring_clone_isolated: yes
ok
