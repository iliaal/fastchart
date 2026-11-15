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
 * distance jitter path; the 512-node ring is the review_perf fixture.
 *
 * TEMPORARY DIAGNOSTICS (PR #109): when a recorded digest misses, or when
 * the packed path disagrees with FASTCHART_NETWORK_SCALAR=1, print compiler
 * flags, both digests, and the first differing node coordinates. Revert
 * this after the gcc-lane mismatch is classified. Passing output is
 * unchanged so currently-green lanes stay green. */
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
function render_pts(int $n, string $kind, int $w, int $h, int $seed, int $it): array {
    [$nodes, $links] = fixture($n, $kind);
    $svg = (new FastChart\NetworkChart($w, $h))
        ->setSeed($seed)->setIterations($it)
        ->setNodes($nodes)->setLinks($links)->renderSvg();
    return centers($svg);
}
function layout_digest(array $pts): string {
    return md5(implode(';', array_map(fn($p) => "$p[0],$p[1]", $pts)));
}
function env_set(string $name, ?string $value): void {
    if ($value === null) {
        putenv($name);
    } else {
        putenv("$name=$value");
    }
}

$cases = [
    ['ring17_512', 512, 'ring17', 1000, 800, 424242, 300, 'd4e6cb5b20c46fc745a1e7c2f73e0383'],
    ['chain_33_small', 33, 'chain', 130, 120, 7, 300, 'd773c0b6d5265cf8279a766f4bbfbff6'],
    ['star_64', 64, 'star', 200, 200, 11, 60, '40b7fde84f1dd5db12b46bff4ee53dc4'],
    ['isolated_127', 127, 'none', 420, 300, 3, 300, '1d9bdcea851a6feda10722a2c27390d9'],
];
$compare = ['ring17_512', 'isolated_127'];
$saved = [];
foreach ($cases as [$name, $n, $kind, $w, $h, $seed, $it, $want]) {
    $pts = render_pts($n, $kind, $w, $h, $seed, $it);
    $inside = true;
    foreach ($pts as [$x, $y]) {
        if ($x < 0 || $y < 0 || $x > $w || $y > $h) $inside = false;
    }
    $got = layout_digest($pts);
    echo $name, ': nodes=', count($pts), ' inside=', $inside ? 'yes' : 'NO',
        ' layout=', $got === $want ? 'reference' : "DIFFERS ($got)", "\n";
    if (in_array($name, $compare, true)) {
        $saved[$name] = compact('n', 'kind', 'w', 'h', 'seed', 'it', 'want', 'pts', 'got');
    }
}

$diag = false;
$scalar = [];
env_set('FASTCHART_NETWORK_SCALAR', '1');
foreach ($saved as $name => $c) {
    $pts = render_pts($c['n'], $c['kind'], $c['w'], $c['h'], $c['seed'], $c['it']);
    $got = layout_digest($pts);
    $scalar[$name] = ['pts' => $pts, 'got' => $got];
    if ($c['got'] !== $c['want'] || $c['got'] !== $got) {
        $diag = true;
    }
}
env_set('FASTCHART_NETWORK_SCALAR', null);

if ($diag) {
    env_set('FASTCHART_NETWORK_DIAG', '1');
    /* Tiny render so the extension prints compiler identity once. */
    render_pts(2, 'none', 200, 200, 1, 1);
    env_set('FASTCHART_NETWORK_DIAG', null);

    echo 'DIAG php=', PHP_VERSION, ' uname=', php_uname(), "\n";
    if (is_file('/etc/os-release')) {
        $os = parse_ini_file('/etc/os-release');
        if (is_array($os)) {
            echo 'DIAG os=', $os['PRETTY_NAME'] ?? $os['NAME'] ?? 'unknown', "\n";
        }
    }
    foreach (['CC', 'CFLAGS', 'CFLAGS_CLEAN', 'CPPFLAGS', 'EXTRA_CFLAGS'] as $k) {
        $v = getenv($k);
        if ($v !== false && $v !== '') {
            echo "DIAG env $k=$v\n";
        }
    }
    foreach (['Makefile', '../Makefile', 'config.nice', '../config.nice'] as $f) {
        if (!is_file($f)) continue;
        if (str_ends_with($f, 'Makefile')) {
            foreach (file($f) as $line) {
                if (preg_match('/^(CC|CFLAGS|CFLAGS_CLEAN)\s*=/', $line)) {
                    echo "DIAG $f: ", rtrim($line), "\n";
                }
            }
        } else {
            echo "DIAG $f: ", str_replace("\n", " | ", trim(file_get_contents($f))), "\n";
        }
    }
    foreach (['config.log', '../config.log'] as $f) {
        if (!is_file($f)) continue;
        $n = 0;
        foreach (file($f) as $line) {
            if (!preg_match('/gcc version|clang version|^CFLAGS\s*=|^CC\s*=/', $line)) continue;
            echo "DIAG $f: ", rtrim($line), "\n";
            if (++$n >= 8) break;
        }
    }

    foreach ($saved as $name => $c) {
        $s = $scalar[$name];
        echo "DIAG $name recorded=", $c['want'], "\n";
        echo "DIAG $name head=", $c['got'],
            " match_recorded=", $c['got'] === $c['want'] ? 'yes' : 'no', "\n";
        echo "DIAG $name scalar=", $s['got'],
            " match_recorded=", $s['got'] === $c['want'] ? 'yes' : 'no',
            " match_head=", $s['got'] === $c['got'] ? 'yes' : 'no', "\n";
        $diffs = 0;
        $maxd = 0;
        $first = null;
        $ncmp = min(count($c['pts']), count($s['pts']));
        for ($i = 0; $i < $ncmp; $i++) {
            if ($c['pts'][$i] === $s['pts'][$i]) continue;
            $d = abs($c['pts'][$i][0] - $s['pts'][$i][0])
               + abs($c['pts'][$i][1] - $s['pts'][$i][1]);
            if ($d > $maxd) $maxd = $d;
            if ($first === null) $first = $i;
            $diffs++;
            if ($diffs <= 5) {
                echo "DIAG $name diff[$i] head=", $c['pts'][$i][0], ',', $c['pts'][$i][1],
                    " scalar=", $s['pts'][$i][0], ',', $s['pts'][$i][1], "\n";
            }
        }
        if (count($c['pts']) !== count($s['pts'])) {
            echo "DIAG $name node_count head=", count($c['pts']),
                " scalar=", count($s['pts']), "\n";
        }
        echo "DIAG $name head_vs_scalar diffs=$diffs",
            $first === null ? " (identical integer centers)" : " first=$first max_abs_l1=$maxd",
            "\n";
    }
}
?>
--EXPECT--
ring17_512: nodes=512 inside=yes layout=reference
chain_33_small: nodes=33 inside=yes layout=reference
star_64: nodes=64 inside=yes layout=reference
isolated_127: nodes=127 inside=yes layout=reference
