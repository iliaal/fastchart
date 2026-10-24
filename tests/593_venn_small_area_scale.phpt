--TEST--
VennDiagram preserves visible geometry for uniformly small positive areas
--EXTENSIONS--
fastchart
--FILE--
<?php
function geo(array $sizes, array $overlaps, float $factor): array
{
    $sets = [];
    foreach ($sizes as $i => $size) {
        $sets[] = ['label' => chr(65 + $i), 'size' => $size * $factor];
    }
    $intersections = [];
    foreach ($overlaps as [$a, $b, $size]) {
        $intersections[] = ['sets' => [$a, $b], 'size' => $size * $factor];
    }
    $svg = (new FastChart\VennDiagram(400, 300))
        ->setSets($sets)->setIntersections($intersections)->renderSvg();
    preg_match_all('/<circle[^>]*cx="([^"]+)"[^>]*cy="([^"]+)"[^>]*r="([^"]+)"/',
        $svg, $matches, PREG_SET_ORDER);
    return array_map(fn($circle) => array_map('floatval', array_slice($circle, 1)), $matches);
}

$cases = [
    [[1, 1], []],
    [[1, 1], [[0, 1, .25]]],
    [[1, 1], [[0, 1, 1]]],
    [[1, .5], [[0, 1, .2]]],
    [[1, 1, 1], []],
    [[1, 1, 1], [[0, 1, .25], [0, 2, .25], [1, 2, .25]]],
    [[1, 1, 1], [[0, 1, 1], [0, 2, .25], [1, 2, .25]]],
    [[1, 1, 1], [[0, 1, 1], [0, 2, .25], [1, 2, .9]]],
];

function nearGeometry(array $a, array $b): bool {
    if (count($a) !== count($b) || !$a) return false;
    foreach ($a as $i => $circle) {
        if ($b[$i][2] <= 0) return false;
        foreach ($circle as $j => $value) {
            if (!is_finite($b[$i][$j]) || abs($value - $b[$i][$j]) > 1) return false;
        }
    }
    return true;
}
$passed = 0;
foreach ($cases as [$sizes, $inters]) {
    $reference = geo($sizes, $inters, 1.0);
    // Preserve the scale-independent layout, allowing integer pixel rounding.
    foreach ([0.5, 1e-10, 1e-20, 1e-100, 1e-300, 1.0 - PHP_FLOAT_EPSILON,
              1.0, 1.0 + PHP_FLOAT_EPSILON] as $factor) {
        if (nearGeometry($reference, geo($sizes, $inters, $factor))) $passed++;
    }
}
echo "scaled layouts: $passed/64\n";
// Construct the smallest positive double directly, without an underflowing scale.
echo 'subnormal visible: ', nearGeometry(geo([1, 1], [], 1), geo([1, 1], [], 5e-324)) ? 'yes' : 'no', "\n";
echo 'subnormal containment: ', nearGeometry(geo([1, 1], [[0, 1, 1]], 1), geo([1, 1], [[0, 1, 1]], 5e-324)) ? 'yes' : 'no', "\n";
?>
--EXPECT--
scaled layouts: 64/64
subnormal visible: yes
subnormal containment: yes
