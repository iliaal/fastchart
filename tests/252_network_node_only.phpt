--TEST--
NetworkChart: a node-only graph (no links) renders instead of throwing
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_9922dc25: render required link_count > 0, rejecting valid isolated-node
 * graphs. Only node_count > 0 is now required; the edge loops no-op at zero
 * links. */

use FastChart\NetworkChart;

$svg = (new NetworkChart())->setSize(400, 300)
    ->setNodes([['id' => 'a', 'label' => 'A'], ['id' => 'b', 'label' => 'B']])
    ->renderSvg();

echo "node_only_renders: ", (strlen($svg) > 500 ? 'yes' : 'no'), "\n";

?>
--EXPECT--
node_only_renders: yes
