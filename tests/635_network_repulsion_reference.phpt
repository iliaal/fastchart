--TEST--
NetworkChart packed repulsion stays identical to the scalar loop, including coincident-node jitter
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
/* Mirrors FC_NET_PACKED in fastchart_network.c: GCC/Clang x86-64 and
 * AArch64, MSVC x64, MSVC x86 with SSE2 (the MSVC default). Elsewhere
 * both renders would take the scalar loop and compare it with itself. */
$m = strtolower(php_uname('m'));
if (PHP_OS_FAMILY === 'Windows') {
    if (strpos($m, 'arm') !== false) die('skip no packed repulsion on Windows ARM');
} elseif (PHP_INT_SIZE !== 8
        || !in_array($m, ['x86_64', 'amd64', 'aarch64', 'arm64'], true)) {
    die('skip build has no packed repulsion');
}
?>
--FILE--
<?php
/* In-process packed vs scalar comparison. FASTCHART_NETWORK_SCALAR=1
 * forces the pre-packed pair loop in the same binary, so a compiler that
 * contracts multiply-add (FMA) is compared against itself rather than
 * against digests recorded on a different machine. The 130x120 chain
 * clamps nodes onto the plot edges, where they coincide and take the
 * sub-0.01 distance jitter path; the 512-node ring is the review_perf
 * fixture. */
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
function render_svg(int $n, string $kind, int $w, int $h, int $seed, int $it): string {
    [$nodes, $links] = fixture($n, $kind);
    return (new FastChart\NetworkChart($w, $h))
        ->setSeed($seed)->setIterations($it)
        ->setNodes($nodes)->setLinks($links)->renderSvg();
}
function env_set(string $name, ?string $value): void {
    if ($value === null) {
        putenv($name);
    } else {
        putenv("$name=$value");
    }
}

$cases = [
    ['ring17_512', 512, 'ring17', 1000, 800, 424242, 300],
    ['chain_33_small', 33, 'chain', 130, 120, 7, 300],
    ['star_64', 64, 'star', 200, 200, 11, 60],
    ['isolated_127', 127, 'none', 420, 300, 3, 300],
];
foreach ($cases as [$name, $n, $kind, $w, $h, $seed, $it]) {
    env_set('FASTCHART_NETWORK_SCALAR', null);
    $packed_svg = render_svg($n, $kind, $w, $h, $seed, $it);
    env_set('FASTCHART_NETWORK_SCALAR', '1');
    $scalar_svg = render_svg($n, $kind, $w, $h, $seed, $it);
    env_set('FASTCHART_NETWORK_SCALAR', null);

    $packed = centers($packed_svg);
    $scalar = centers($scalar_svg);
    $inside = true;
    foreach ($packed as [$x, $y]) {
        if ($x < 0 || $y < 0 || $x > $w || $y > $h) $inside = false;
    }
    $same_svg = $packed_svg === $scalar_svg;
    $diffs = 0;
    $first = null;
    $ncmp = min(count($packed), count($scalar));
    for ($i = 0; $i < $ncmp; $i++) {
        if ($packed[$i] === $scalar[$i]) continue;
        if ($first === null) $first = $i;
        $diffs++;
    }
    if (count($packed) !== count($scalar)) {
        $diffs += abs(count($packed) - count($scalar));
    }
    $match = $same_svg && $diffs === 0 && count($packed) === $n;
    echo $name, ': nodes=', count($packed), ' inside=', $inside ? 'yes' : 'NO',
        ' packed_vs_scalar=', $match ? 'identical' : "DIFFERS (svg=" .
        ($same_svg ? 'same' : 'diff') . " centers=$diffs" .
        ($first === null ? '' : " first=$first packed={$packed[$first][0]},{$packed[$first][1]} scalar={$scalar[$first][0]},{$scalar[$first][1]}") .
        ")", "\n";
}
?>
--EXPECT--
ring17_512: nodes=512 inside=yes packed_vs_scalar=identical
chain_33_small: nodes=33 inside=yes packed_vs_scalar=identical
star_64: nodes=64 inside=yes packed_vs_scalar=identical
isolated_127: nodes=127 inside=yes packed_vs_scalar=identical
