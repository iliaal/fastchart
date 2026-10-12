<?php
/* Run with either the baseline or candidate extension loaded. Reports median
 * milliseconds per render and an SVG hash; hierarchy construction is untimed. */
function tree(int $depth, int $fanout): array {
    if ($depth === 0) return ['value' => 1];
    return ['children' => array_fill(0, $fanout, tree($depth - 1, $fanout))];
}
foreach ([[3, 3], [5, 3], [6, 3], [10, 2]] as [$depth, $fanout]) {
    $chart = (new FastChart\SunburstChart(800, 600))->setHierarchy(tree($depth, $fanout));
    for ($i = 0; $i < 3; $i++) $chart->renderSvg();
    $times = [];
    for ($i = 0; $i < 11; $i++) {
        $t = hrtime(true);
        for ($j = 0; $j < 20; $j++) $svg = $chart->renderSvg();
        $times[] = (hrtime(true) - $t) / 20 / 1e6;
    }
    sort($times);
    printf("%d,%d %.4f %s\n", $depth, $fanout, $times[5], hash('sha256', $svg));
}
