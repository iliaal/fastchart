--TEST--
Label collections charge only retained entries to the 64 KiB budget without replacing prior state
--EXTENSIONS--
fastchart
--FILE--
<?php

$full = str_repeat('A', 8192);
$labels = array_fill(0, 8, $full);
$dicts = fn(array $extra, array $ls) => array_map(
    fn($l) => ['label' => $l] + $extra, $ls);
$over = [...$labels, 'x'];
$columns = fn(array $ls) => array_map(fn($pair) => [
    'label' => $pair[0],
    'segments' => array_slice($pair, 1)
        ? array_map(fn($l) => ['label' => $l, 'value' => 1], array_slice($pair, 1))
        : [['value' => 1]],
], array_chunk($ls, 2));
$box = ['min' => 1, 'q1' => 2, 'median' => 3, 'q3' => 4, 'max' => 5];

$cases = [
    'gantt' => [
        fn() => new FastChart\GanttChart(300, 200),
        fn($c, $ls) => $c->setTasks(array_map(
            fn($l) => ['name' => $l, 'start' => 0, 'end' => 1], $ls)),
    ],
    'boxplot' => [
        fn() => new FastChart\BoxPlot(300, 200),
        fn($c, $ls) => $c->setBoxes($dicts($box, $ls)),
    ],
    'treemap' => [
        fn() => new FastChart\Treemap(300, 200),
        fn($c, $ls) => $c->setItems($dicts(['value' => 1], $ls)),
    ],
    'funnel' => [
        fn() => new FastChart\Funnel(300, 200),
        fn($c, $ls) => $c->setStages($dicts(['value' => 1], $ls)),
    ],
    'waterfall' => [
        fn() => new FastChart\Waterfall(300, 200),
        fn($c, $ls) => $c->setBars($dicts(['value' => 1], $ls)),
    ],
    'pareto' => [
        fn() => new FastChart\ParetoChart(300, 200),
        fn($c, $ls) => $c->setBars($dicts(['value' => 1], $ls)),
    ],
    'sankey' => [
        fn() => new FastChart\SankeyChart(300, 200),
        fn($c, $ls) => $c->setNodes($dicts([], $ls)),
        fn($c) => $c->setLinks([['from' => 0, 'to' => 1, 'value' => 1]]),
    ],
    'marimekko' => [
        fn() => new FastChart\MarimekkoChart(300, 200),
        fn($c, $ls) => $c->setColumns($columns($ls)),
    ],
];

foreach ($cases as $name => $case) {
    [$make, $set] = $case;
    $set($make(), $labels);
    $chart = $make();
    $set($chart, ['kept', 'kept2', 'kept3', 'kept4']);
    if (isset($case[2])) $case[2]($chart);
    $before = $chart->renderSvg();
    try {
        $set($chart, $over);
        echo "$name: NOT REJECTED\n";
    } catch (ValueError $e) {
        echo "$name: ",
            str_contains($e->getMessage(), 'aggregate text exceeds the 65536-byte limit')
                ? 'rejected' : $e->getMessage(),
            ', state ', $chart->renderSvg() === $before ? 'preserved' : 'CHANGED',
            "\n";
    }
}

/* A dropped entry's label must not count: 8 retained full labels plus
 * one entry the setter discards still fit the budget. */
$dropped = [
    'gantt' => fn($c) => $c->setTasks([
        ...array_map(fn($l) => ['name' => $l, 'start' => 0, 'end' => 1], $labels),
        ['name' => $full]]),
    'boxplot' => fn($c) => $c->setBoxes([...$dicts($box, $labels),
        ['label' => $full, 'min' => 5, 'q1' => 1, 'median' => 3, 'q3' => 4, 'max' => 5]]),
    'treemap' => fn($c) => $c->setItems([...$dicts(['value' => 1], $labels),
        ['label' => $full, 'value' => 0]]),
    'funnel' => fn($c) => $c->setStages([...$dicts(['value' => 1], $labels),
        ['label' => $full, 'value' => 0]]),
    'waterfall' => fn($c) => $c->setBars([...$dicts(['value' => 1], $labels),
        ['label' => $full, 'value' => NAN]]),
    'pareto' => fn($c) => $c->setBars([...$dicts(['value' => 1], $labels),
        ['label' => $full, 'value' => -1]]),
    'marimekko' => fn($c) => $c->setColumns([...$columns($labels),
        ['label' => $full, 'segments' => [['label' => $full, 'value' => 0]]]]),
];
foreach ($dropped as $name => $set) {
    try {
        $set($cases[$name][0]());
        echo "$name dropped entry: not counted\n";
    } catch (ValueError $e) {
        echo "$name dropped entry: ", $e->getMessage(), "\n";
    }
}
?>
--EXPECT--
gantt: rejected, state preserved
boxplot: rejected, state preserved
treemap: rejected, state preserved
funnel: rejected, state preserved
waterfall: rejected, state preserved
pareto: rejected, state preserved
sankey: rejected, state preserved
marimekko: rejected, state preserved
gantt dropped entry: not counted
boxplot dropped entry: not counted
treemap dropped entry: not counted
funnel dropped entry: not counted
waterfall dropped entry: not counted
pareto dropped entry: not counted
marimekko dropped entry: not counted
