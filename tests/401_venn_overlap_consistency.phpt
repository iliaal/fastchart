--TEST--
VennDiagram does not silently ignore an incompatible third pairwise overlap
--EXTENSIONS--
fastchart
--FILE--
<?php

function venn(float $bc): string
{
    return (new FastChart\VennDiagram(400, 300))
        ->setSets([
            ['label' => 'A', 'size' => 100],
            ['label' => 'B', 'size' => 100],
            ['label' => 'C', 'size' => 100],
        ])
        ->setIntersections([
            ['sets' => [0, 1], 'size' => 100],
            ['sets' => [0, 2], 'size' => 30],
            ['sets' => [1, 2], 'size' => $bc],
        ])
        ->renderSvg();
}

echo 'incompatible overlap changes layout: ',
    venn(30) !== venn(90) ? "yes\n" : "no\n";

?>
--EXPECT--
incompatible overlap changes layout: yes
