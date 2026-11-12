--TEST--
Sankey link compaction preserves filtering, order, replacements and clones
--EXTENSIONS--
fastchart
--FILE--
<?php
function chart(string $class): FastChart\Chart {
    $chart = (new $class(320, 240))->setNodes([
        ['color' => 0xDD2200], ['color' => 0x00AA44], ['color' => 0x2266FF],
    ]);
    return $chart;
}
function output(FastChart\Chart $chart): string {
    try {
        return $chart->renderSvg();
    } catch (Error $e) {
        return get_class($e) . ': ' . $e->getMessage();
    }
}
$from = 0;
$to = 2;
$value = 1.5;
$referenced = ['from' => &$from, 'to' => &$to, 'value' => &$value];
$first = ['from' => 0, 'to' => 1, 'value' => 3];
$mixed = [null, false, ['from' => 0], ['from' => '0', 'to' => 1, 'value' => 2],
    ['from' => -1, 'to' => 0, 'value' => 2], ['from' => 0, 'to' => 3, 'value' => 2],
    ['from' => 0, 'to' => 0, 'value' => 2], ['from' => 0, 'to' => 1, 'value' => 0],
    ['from' => 0, 'to' => 1, 'value' => NAN], ['from' => 0, 'to' => 1, 'value' => INF],
    'numeric-string' => ['from' => 1, 'to' => 2, 'value' => '2.5'],
    'first' => $first, 'reference' => &$referenced,
    'last' => ['from' => 1, 'to' => 2, 'value' => 2.5]];
$accepted = [$first, ['from' => 0, 'to' => 2, 'value' => 1.5],
    ['from' => 1, 'to' => 2, 'value' => 2.5]];
$full = array_fill(0, 1024, $first);
foreach (['SankeyChart'] as $name) {
    $class = 'FastChart\\' . $name;
    $chart = chart($class);
    foreach ([[$full, $full], [$mixed, $accepted], [[], []], [[null], []],
              [[$first], [$first]], [$full, $full], [$mixed, $accepted]] as $i => [$input, $expected]) {
        $reference = output(chart($class)->setLinks($expected));
        if ($expected !== [] && !str_contains($reference, '<svg')) {
            echo "$name replacement $i did not render SVG\n";
        }
        $fluent = $chart->setLinks($input) === $chart;
        $copy = clone $chart;
        $actual = output($chart);
        $chart->setLinks([]);
        if (!$fluent || $actual !== $reference || output($copy) !== $reference
            || output($copy) !== $actual) echo "$name replacement $i: BAD\n";
        $chart = $copy;
    }
    $before = output($chart);
    try {
        $chart->setLinks(array_fill(0, 1025, null));
        echo "$name cap did not throw\n";
    } catch (ValueError $e) {
        if (output($chart) !== $before) echo "$name cap changed state\n";
    }
    $chart->setNodes([[], [], []]);
    if (output($chart) !== output(chart($class)->setNodes([[], [], []]))) {
        echo "$name setNodes did not clear links\n";
    }
    echo "$name replacements checked\n";
}
?>
--EXPECT--
SankeyChart replacements checked
