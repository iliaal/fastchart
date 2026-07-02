--TEST--
SankeyChart: huge finite link values are dropped before layout totals overflow
--EXTENSIONS--
fastchart
--FILE--
<?php

$nodes = [];
$links = [];
for ($i = 0; $i < 30; $i++) {
    $nodes[] = ['label' => "S$i"];
    $links[] = ['from' => $i, 'to' => 30, 'value' => 1.5e308];
}
$nodes[] = ['label' => 'Sink'];
$links[] = ['from' => 0, 'to' => 30, 'value' => 1.0];

$svg = (new FastChart\SankeyChart(800, 200))
    ->setNodes($nodes)
    ->setLinks($links)
    ->renderSvg();

echo "ribbon_count: ", substr_count($svg, '<polygon'), "\n";
echo "finite_svg: ", (preg_match('/nan|inf/i', $svg) ? 'NO' : 'yes'), "\n";
echo "closed_svg: ", (str_contains($svg, '</svg>') ? 'yes' : 'NO'), "\n";

?>
--EXPECT--
ribbon_count: 1
finite_svg: yes
closed_svg: yes
