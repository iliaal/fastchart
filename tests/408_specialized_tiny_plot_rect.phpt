--TEST--
Specialized renderers handle tiny explicit plot rectangles without overflow or leaks
--EXTENSIONS--
fastchart
--FILE--
<?php

$charts = [
    'contour' => (new FastChart\ContourChart(200, 150))
        ->setGrid([[0, 1], [1, 2]])
        ->setFilled(true),
    'chord' => (new FastChart\ChordDiagram(200, 150))
        ->setNodes([[], []])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 1]]),
    'sankey' => (new FastChart\SankeyChart(200, 150))
        ->setNodes([[], []])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 1]]),
];

foreach ($charts as $name => $chart) {
    $svg = $chart->setPlotRect(1, 1, 5, 5)->renderSvg();
    $finite = stripos($svg, 'nan') === false && stripos($svg, 'inf') === false;
    echo "$name: ", str_starts_with($svg, '<?xml') && $finite ? "ok\n" : "bad\n";
}

?>
--EXPECT--
contour: ok
chord: ok
sankey: ok
