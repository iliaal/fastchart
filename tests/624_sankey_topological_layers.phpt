--TEST--
SankeyChart preserves longest-path layers, parallel links and cycle rejection
--EXTENSIONS--
fastchart
--FILE--
<?php
use FastChart\SankeyChart;

function checkGraph(int $count, array $edges): void {
    $layers = array_fill(0, $count, 0);
    /* Independent fixed-point oracle, including disconnected components. */
    for ($pass = 0; $pass < $count; $pass++) {
        $changed = false;
        foreach ($edges as [$from, $to]) {
            if ($layers[$from] + 1 > $layers[$to]) {
                $layers[$to] = $layers[$from] + 1;
                $changed = true;
            }
        }
        if (!$changed) break;
    }
    $cyclic = $changed;
    $nodes = [];
    for ($i = 0; $i < $count; $i++) $nodes[] = ['color' => 0x100000 + $i];
    $links = [];
    foreach ($edges as [$from, $to]) {
        $links[] = ['from' => $from, 'to' => $to, 'value' => 1];
    }
    $chart = (new SankeyChart(1200, 1200))->setNodes($nodes)->setLinks($links);
    try {
        $svg = $chart->renderSvg();
    } catch (Error $e) {
        if (!$cyclic || $e->getMessage() !== 'FastChart\\SankeyChart::draw() requires an acyclic flow graph') {
            throw $e;
        }
        return;
    }
    if ($cyclic) throw new RuntimeException('cycle rendered');
    $step = (1200 - 280 - 14) / max(1, max($layers));
    preg_match_all('/<rect x="(\d+)" y="\d+" width="14" height="\d+" fill="#([0-9A-F]{6})"\/>/', $svg, $rects, PREG_SET_ORDER);
    if (count($rects) !== $count) throw new RuntimeException('missing nodes');
    foreach ($rects as $i => $rect) {
        if (hexdec($rect[2]) !== 0x100000 + $i || (int)$rect[1] !== (int)(140 + $step * $layers[$i])) {
            throw new RuntimeException('wrong node order or layer');
        }
    }
    preg_match_all('/<polygon [^>]+ fill="#([0-9A-F]{6})"\/>/', $svg, $ribbons);
    if (count($ribbons[1]) !== count($edges)) {
        throw new RuntimeException('parallel links lost');
    }
    foreach ($edges as $i => [$from, $to]) {
        if (hexdec($ribbons[1][$i]) !== 0x100000 + $from) {
            throw new RuntimeException('ribbon order changed');
        }
    }
    if ($svg !== $chart->renderSvg() || $svg !== (clone $chart)->renderSvg()) {
        throw new RuntimeException('render mutated graph');
    }
}

$possible = [[0, 1], [0, 2], [1, 0], [1, 2], [2, 0], [2, 1]];
for ($mask = 1; $mask < 64; $mask++) {
    $edges = [];
    foreach ($possible as $bit => $edge) {
        if ($mask & (1 << $bit)) $edges[] = $edge;
    }
    checkGraph(3, $edges);
}
echo "all 63 nonempty three-node graphs: ok\n";

/* A short direct path must not hide a longer path to the same sink. */
$edges = [[0, 4], [3, 4], [2, 3], [1, 2], [0, 1], [5, 3], [5, 3]];
checkGraph(8, $edges);
checkGraph(8, array_reverse($edges));
echo "unequal paths, parallel edges and isolated nodes: ok\n";

checkGraph(7, [[0, 1], [1, 2], [3, 4], [4, 3], [4, 5]]);
checkGraph(5, [[0, 1], [1, 2], [2, 3], [3, 2], [3, 4]]);
echo "disconnected and reachable cycles: ok\n";

$edges = [];
for ($i = 254; $i >= 0; $i--) $edges[] = [$i, $i + 1];
checkGraph(256, $edges);
checkGraph(256, array_reverse($edges));
echo "maximum-length chain in both link orders: ok\n";

checkGraph(2, array_fill(0, 1024, [0, 1]));
checkGraph(256, [[254, 255]]);
echo "maximum parallel links and source queue: ok\n";
?>
--EXPECT--
all 63 nonempty three-node graphs: ok
unequal paths, parallel edges and isolated nodes: ok
disconnected and reachable cycles: ok
maximum-length chain in both link orders: ok
maximum parallel links and source queue: ok
