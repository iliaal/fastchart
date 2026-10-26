--TEST--
VennDiagram preserves tangent and infeasible layouts across area scales
--EXTENSIONS--
fastchart
--FILE--
<?php
function circles(array $overlaps, float $factor = 1.0, array $sizes = [.136, .055, .787]): array
{
    $sets = [];
    foreach ($sizes as $i => $size) {
        $sets[] = ['label' => chr(65 + $i), 'size' => $size * $factor];
    }
    foreach ($overlaps as &$overlap) $overlap['size'] *= $factor;
    unset($overlap);
    $svg = (new FastChart\VennDiagram(400, 300))
        ->setSets($sets)->setIntersections($overlaps)->renderSvg();
    preg_match_all('/<circle[^>]*cx="([^"]+)"[^>]*cy="([^"]+)"[^>]*r="([^"]+)"/',
        $svg, $matches, PREG_SET_ORDER);
    return array_map(fn($circle) => array_map('intval', array_slice($circle, 1)), $matches);
}
// A is internally tangent to C, and B is externally tangent to both.
// Height-squared cancellation must not choose a different branch solely
// because all set and intersection areas were scaled together.
$actual = circles([['sets' => [0, 2], 'size' => .136]]);
$expected = [[237, 153, 48], [237, 153, 48], [317, 153, 30],
             [317, 153, 30], [169, 154, 117], [169, 154, 117]];
$preserved = count($actual) === count($expected);
foreach ($expected as $i => $circle) {
    foreach ($circle as $j => $value) {
        // Rendering rounds to integer pixels; tolerate a one-pixel difference.
        if (!isset($actual[$i][$j]) || abs($actual[$i][$j] - $value) > 1) {
            $preserved = false;
        }
    }
}
echo 'fractional tangent layout: ', $preserved ? 'preserved' : 'changed', "\n";
function sameLayout(array $actual, array $expected): bool
{
    if (count($actual) !== count($expected)) return false;
    foreach ($expected as $i => $circle) {
        foreach ($circle as $j => $value) {
            if (!isset($actual[$i][$j]) || abs($actual[$i][$j] - $value) > 1) return false;
        }
    }
    return true;
}
$tangent = $infeasible = 0;
$impossible = [
    ['sets' => [0, 1], 'size' => .99],
    ['sets' => [0, 2], 'size' => .25],
    ['sets' => [1, 2], 'size' => .30],
];
// These pairwise distances cannot form a triangle. They must stay in the
// fallback branch, not become "co-located" when an absolute tolerance wins.
$fallback = [[200, 206, 64], [200, 206, 64], [138, 101, 64],
             [138, 101, 64], [261, 101, 64], [261, 101, 64]];
for ($exponent = 3; $exponent >= -13; $exponent--) {
    $factor = 10 ** $exponent;
    if (sameLayout(circles([['sets' => [0, 2], 'size' => .136]], $factor), $expected)) $tangent++;
    if (sameLayout(circles($impossible, $factor, [1, 1, 1]), $fallback)) $infeasible++;
}
echo "tangent scales: $tangent/17\n";
echo "infeasible scales: $infeasible/17\n";
?>
--EXPECT--
fractional tangent layout: preserved
tangent scales: 17/17
infeasible scales: 17/17
