--TEST--
SankeyChart: a cyclic flow graph is rejected; an acyclic one renders
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_5666a849: cyclic links never converged in the layer pass, producing an
 * order-dependent backward-ribbon layout. Cycles are now detected and rejected. */

use FastChart\SankeyChart;

try {
    (new SankeyChart())->setSize(400, 300)
        ->setNodes([['label' => 'A'], ['label' => 'B']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 5], ['from' => 1, 'to' => 0, 'value' => 3]])
        ->renderSvg();
    echo "cyclic: rendered\n";
} catch (\Throwable $e) {
    echo "cyclic: ", (strpos($e->getMessage(), 'acyclic') !== false ? 'rejected' : 'threw-other'), "\n";
}

$svg = (new SankeyChart())->setSize(400, 300)
    ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
    ->setLinks([['from' => 0, 'to' => 1, 'value' => 5], ['from' => 1, 'to' => 2, 'value' => 3]])
    ->renderSvg();
echo "acyclic_renders: ", (strlen($svg) > 500 ? 'yes' : 'no'), "\n";

?>
--EXPECT--
cyclic: rejected
acyclic_renders: yes
