--TEST--
Gantt, Sankey, and stream Area render valid geometry at accepted numeric extremes
--EXTENSIONS--
fastchart
--FILE--
<?php

$gantt = (new FastChart\GanttChart(400, 220))
    ->setTasks([[
        'name' => 'edge',
        'start' => PHP_INT_MAX,
        'end' => PHP_INT_MAX,
    ]])
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->renderSvg();
echo 'gantt ticks: ', substr_count($gantt, '<text') > 1 ? "yes\n" : "no\n";

$sankey = (new FastChart\SankeyChart(400, 200))
    ->setNodes([['label' => 'A'], ['label' => 'B']])
    ->setLinks([['from' => 0, 'to' => 1, 'value' => 5e-324]])
    ->renderSvg();
echo 'sankey ribbon: ', substr_count($sankey, '<polygon') >= 1 ? "yes\n" : "no\n";

$stream = static fn (float $value): string =>
    (new FastChart\AreaChart(300, 200))
        ->setSeries([
            ['data' => [1, $value, 2]],
            ['data' => [2, 3, 4]],
        ])
        ->setStreamMode(true)
        ->renderSvg();
echo 'stream negative clamped: ', $stream(-100) === $stream(0) ? "yes\n" : "no\n";

?>
--EXPECT--
gantt ticks: yes
sankey ribbon: yes
stream negative clamped: yes
