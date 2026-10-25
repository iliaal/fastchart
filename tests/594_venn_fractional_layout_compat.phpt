--TEST--
VennDiagram preserves ordinary fractional tangent layout arithmetic
--EXTENSIONS--
fastchart
--FILE--
<?php
function circles(array $overlaps): array
{
    $svg = (new FastChart\VennDiagram(400, 300))
        ->setSets([
            ['label' => 'A', 'size' => .136],
            ['label' => 'B', 'size' => .055],
            ['label' => 'C', 'size' => .787],
        ])
        ->setIntersections($overlaps)->renderSvg();
    preg_match_all('/<circle[^>]*cx="([^"]+)"[^>]*cy="([^"]+)"[^>]*r="([^"]+)"/',
        $svg, $matches, PREG_SET_ORDER);
    return array_map(fn($circle) => array_map('intval', array_slice($circle, 1)), $matches);
}
// A is internally tangent to C, and B is externally tangent to both.
// Arbitrary area normalization made the tiny height-squared cancellation
// negative and incorrectly selected the visibly different fallback triad.
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
?>
--EXPECT--
fractional tangent layout: preserved
