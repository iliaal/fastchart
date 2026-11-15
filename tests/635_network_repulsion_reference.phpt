--TEST--
NetworkChart repulsion keeps x86-64 layouts identical, including coincident-node jitter
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
/* Digests come from the scalar repulsion loop on an x86-64 build without
 * FMA. Targets whose compilers fuse multiply-add produce other layouts. */
if (!in_array(strtolower(php_uname('m')), ['x86_64', 'amd64'], true)) {
    echo "skip: layout digests are x86-64 specific\n";
}
?>
--FILE--
<?php
/* Node centers are compared with digests recorded from the scalar
 * repulsion loop that predates packed sqrt/divide. Small canvases clamp
 * nodes onto the plot edges, where they coincide and take the sub-0.01
 * distance jitter path; the 512-node ring is the review_perf fixture. */
function centers(string $svg): array {
    preg_match_all('/<circle cx="(-?\d+)" cy="(-?\d+)" r="\d+" fill="#/',
        $svg, $m, PREG_SET_ORDER);
    return array_map(fn($x) => [(int)$x[1], (int)$x[2]], $m);
}
function fixture(int $n, string $kind): array {
    $links = [];
    for ($i = 0; $i < $n; $i++) {
        if ($kind === 'ring17') {
            $links[] = ['from' => $i, 'to' => ($i + 1) % $n, 'value' => 1];
            $links[] = ['from' => $i, 'to' => ($i + 17) % $n, 'value' => 2];
        } elseif ($kind === 'chain' && $i) {
            $links[] = ['from' => $i - 1, 'to' => $i, 'value' => 1];
        } elseif ($kind === 'star' && $i) {
            $links[] = ['from' => 0, 'to' => $i, 'value' => 1];
        }
    }
    return [array_fill(0, $n, []), $links];
}
$cases = [
    ['ring17_512', 512, 'ring17', 1000, 800, 424242, 300, 'd4e6cb5b20c46fc745a1e7c2f73e0383'],
    ['chain_33_small', 33, 'chain', 130, 120, 7, 300, 'd773c0b6d5265cf8279a766f4bbfbff6'],
    ['star_64', 64, 'star', 200, 200, 11, 60, '40b7fde84f1dd5db12b46bff4ee53dc4'],
    ['isolated_127', 127, 'none', 420, 300, 3, 300, '1d9bdcea851a6feda10722a2c27390d9'],
];
foreach ($cases as [$name, $n, $kind, $w, $h, $seed, $it, $want]) {
    [$nodes, $links] = fixture($n, $kind);
    $svg = (new FastChart\NetworkChart($w, $h))
        ->setSeed($seed)->setIterations($it)
        ->setNodes($nodes)->setLinks($links)->renderSvg();
    $pts = centers($svg);
    $inside = true;
    foreach ($pts as [$x, $y]) {
        if ($x < 0 || $y < 0 || $x > $w || $y > $h) $inside = false;
    }
    $got = md5(implode(';', array_map(fn($p) => "$p[0],$p[1]", $pts)));
    echo $name, ': nodes=', count($pts), ' inside=', $inside ? 'yes' : 'NO',
        ' layout=', $got === $want ? 'reference' : "DIFFERS ($got)", "\n";
}
?>
--EXPECT--
ring17_512: nodes=512 inside=yes layout=reference
chain_33_small: nodes=33 inside=yes layout=reference
star_64: nodes=64 inside=yes layout=reference
isolated_127: nodes=127 inside=yes layout=reference
