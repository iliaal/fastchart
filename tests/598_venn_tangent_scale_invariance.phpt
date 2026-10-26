--TEST--
VennDiagram renders tangent, nested and near-collinear inputs the same at every area scale
--EXTENSIONS--
fastchart
--FILE--
<?php
function circles(array $sizes, array $overlaps, float $factor): array
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
    $out = [];
    // Each set is drawn as a fill circle followed by a stroke circle.
    foreach ($matches as $k => $circle) {
        if ($k % 2 === 0) $out[] = array_map('intval', array_slice($circle, 1));
    }
    return $out;
}

function sameLayout(array $actual, array $expected): bool
{
    if (count($actual) !== count($expected)) return false;
    foreach ($expected as $i => $circle) {
        foreach ($circle as $j => $value) {
            // Rendering rounds to integer pixels; tolerate a one-pixel difference.
            if (abs($actual[$i][$j] - $value) > 1) return false;
        }
    }
    return true;
}

// Each expected layout is collinear: every input below is an exactly or
// nearly degenerate triangle of centres, and must not switch to the
// symmetric fallback at some scales only.
$cases = [
    // A internally tangent to C, B externally tangent to both; the
    // height-squared cancellation is largest because d(A,C) << d(A,B).
    'tangent, containment pair 0-2' => [
        [.727, .969, .73], [[0, 2, .727]],
        [[111, 154, 76], [276, 154, 88], [111, 154, 76]],
    ],
    'tangent, containment pair 0-1' => [
        [.7985085053828119, .7972770710323365, .1054720708799884],
        [[0, 1, .7972770710323365]],
        [[157, 154, 117], [157, 154, 116], [316, 154, 42]],
    ],
    // Chains of internal tangency: C inside B inside A, and A inside B inside C.
    'nested, outer 0' => [
        [.665475675896963, .14426628947689493, .050118996421861926],
        [[1, 2, .050118996421861926], [0, 1, .14426628947689493],
         [0, 2, .050118996421861926]],
        [[200, 154, 117], [262, 154, 54], [284, 154, 32]],
    ],
    'nested, outer 2' => [
        [.19571736168149736, .7636931694176482, .7979774400698847],
        [[0, 1, .19571736168149736], [1, 2, .7636931694176482],
         [0, 2, .19571736168149736]],
        [[140, 154, 57], [197, 154, 114], [200, 154, 117]],
    ],
    // Pairwise distances violate the triangle inequality by about 8 machine
    // epsilons of their sum: inside the noise of rescaled inputs.
    'near-collinear' => [
        [.8130307917823227, .7652930359613583, .7569079984593708],
        [[0, 1, .5645257404086311], [0, 2, .5396801514531828],
         [1, 2, .7395201012850257]],
        [[173, 154, 117], [225, 154, 113], [230, 154, 112]],
    ],
    // C almost fully inside B and A barely overlapping C, both a few
    // epsilons from tangency, where the solved distances are least accurate.
    'near-tangent' => [
        [.20508448394019366, .9038618063618717, .6542096719118812],
        [[1, 2, .654209671911879], [0, 2, 2.9507540463622876e-15]],
        [[87, 154, 53], [253, 154, 112], [236, 154, 95]],
    ],
    // A and B nearly co-located; areas above 1 must use the same geometry
    // tolerances as areas below 1.
    'co-located' => [
        [.6260129865380065, .6260129865380065, .5904088658003178],
        [[0, 1, .6260129482536492], [0, 2, .27144296932587697],
         [1, 2, .2714429693284933]],
        [[150, 154, 115], [150, 154, 115], [253, 154, 112]],
    ],
];
foreach ($cases as $name => [$sizes, $overlaps, $expected]) {
    $matched = 0;
    for ($exponent = 3; $exponent >= -13; $exponent--) {
        if (sameLayout(circles($sizes, $overlaps, 10 ** $exponent), $expected)) $matched++;
    }
    echo "$name: $matched/17\n";
}
?>
--EXPECT--
tangent, containment pair 0-2: 17/17
tangent, containment pair 0-1: 17/17
nested, outer 0: 17/17
nested, outer 2: 17/17
near-collinear: 17/17
near-tangent: 17/17
co-located: 17/17
