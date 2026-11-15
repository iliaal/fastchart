<?php
/* CirclePacking differential harness: one line per render, "<name>\t<md5 of
 * the SVG>". Run it against two builds and diff the results:
 *
 *   php -d extension=/path/base.so bench/circlepack_equivalence.php > base.txt
 *   php -d extension=/path/head.so bench/circlepack_equivalence.php > head.txt
 *   diff base.txt head.txt
 *
 * Covers flat groups of 2-2000 siblings (including groups past the 8M
 * collision-work budget), nested groups at depths 2-4, many equal radii,
 * tiny and huge value spreads, and several canvas sizes. Pass a seed count
 * to scale it: `php bench/circlepack_equivalence.php 400`. */

if (!extension_loaded('fastchart')) {
    fwrite(STDERR, "fastchart extension not loaded\n");
    exit(1);
}
$N = (int)($argv[1] ?? 150);

function value(int $mode): float {
    return match ($mode) {
        0 => mt_rand(1, 1000),
        1 => [5, 5, 5, 20, 80][mt_rand(0, 4)],
        2 => mt_rand(1, 1000000) / 1000.0,
        default => 10 ** (mt_rand(0, 90) / 10),
    };
}

function node(int $depth, int $mode, int &$budget): array {
    if ($depth === 0 || $budget <= 1) {
        return ['label' => "n$budget", 'value' => value($mode)];
    }
    $kids = [];
    $k = mt_rand(1, $depth > 1 ? 8 : 60);
    for ($i = 0; $i < $k && $budget > 1; $i++) {
        $budget--;
        $kids[] = node(mt_rand(0, $depth - 1), $mode, $budget);
    }
    return ['label' => "g$budget", 'children' => $kids];
}

for ($seed = 1; $seed <= $N; $seed++) {
    mt_srand($seed);
    $mode = $seed % 4;
    if ($seed % 3 === 0) {
        $n = [2, 3, 7, 40, 150, 400, 900, 2000][$seed % 8];
        $kids = [];
        for ($i = 0; $i < $n; $i++) $kids[] = ['value' => value($mode)];
        $root = ['children' => $kids];
    } else {
        $budget = 2000;
        $root = node(mt_rand(2, 4), $mode, $budget);
    }
    [$w, $h] = [[400, 400], [800, 600], [1200, 1200], [300, 900]][$seed % 4];
    $svg = (new FastChart\CirclePacking($w, $h))->setHierarchy($root)->renderSvg();
    echo "seed $seed\t", md5($svg), "\n";
}
