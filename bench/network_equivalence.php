<?php
/* NetworkChart layout differential harness: one line per render,
 * "<name>\t<md5 of the SVG>". Run it against two builds and diff:
 *
 *   php -d extension=/path/base.so bench/network_equivalence.php > base.txt
 *   php -d extension=/path/head.so bench/network_equivalence.php > head.txt
 *   diff base.txt head.txt
 *
 * A layout change that must keep output byte-identical should show no
 * differences. Covers 1-300 nodes, chain/star/random/no links, canvases from
 * 1000x800 down to ones small enough that clamped nodes coincide (the
 * sub-0.01 distance jitter path), and 1-300 iterations. Pass a seed count to
 * scale it: `php bench/network_equivalence.php 64`. */

if (!extension_loaded('fastchart')) {
    fwrite(STDERR, "fastchart extension not loaded\n");
    exit(1);
}
$S = (int)($argv[1] ?? 32);

function links_for(string $kind, int $n, int $seed): array {
    mt_srand($seed);
    $l = [];
    for ($i = 1; $i < $n; $i++) {
        switch ($kind) {
        case 'chain': $l[] = ['from' => $i - 1, 'to' => $i, 'value' => 1 + $i % 4]; break;
        case 'star':  $l[] = ['from' => 0, 'to' => $i, 'value' => 1]; break;
        case 'random':
            $l[] = ['from' => mt_rand(0, $n - 1), 'to' => mt_rand(0, $n - 1), 'value' => mt_rand(1, 9)];
            $l[] = ['from' => $i, 'to' => mt_rand(0, $n - 1), 'value' => 1];
            break;
        }
    }
    return $l;
}

$sizes = [[1000, 800], [420, 300], [200, 200], [110, 90]];
$counts = [1, 2, 3, 4, 5, 7, 8, 16, 33, 64, 127, 200, 300];
$iters = [1, 5, 60, 300];
foreach (['chain', 'star', 'random', 'none'] as $kind) {
    foreach ($counts as $n) {
        $nodes = [];
        for ($i = 0; $i < $n; $i++) $nodes[] = ['label' => "n$i"];
        for ($s = 0; $s < $S; $s++) {
            /* Every size x iteration pair, one per seed, cycling. */
            $combo = $s % (count($sizes) * count($iters));
            [$w, $h] = $sizes[$combo % count($sizes)];
            $it = $iters[intdiv($combo, count($sizes))];
            $seed = 1 + $s * 7919 + $n;
            $c = (new FastChart\NetworkChart($w, $h))
                ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
                ->setSeed($seed)->setIterations($it)
                ->setNodes($nodes)->setLinks(links_for($kind, $n, $seed));
            echo "$kind n=$n {$w}x$h it=$it seed=$seed\t", md5($c->renderSvg()), "\n";
        }
    }
}
