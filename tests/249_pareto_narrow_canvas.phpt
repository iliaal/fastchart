--TEST--
ParetoChart: a canvas narrower than the bar count is rejected instead of collapsing bars
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_904b6166: integer slot width collapsed to 0 on a narrow canvas, stacking
 * every bar at one x. The renderer now throws when the plot is narrower than
 * the bar count. */

use FastChart\ParetoChart;

$bars = [];
for ($i = 0; $i < 50; $i++) $bars[] = ['label' => "x$i", 'value' => $i + 1];

try {
    (new ParetoChart())->setSize(121, 200)->setBars($bars)->renderSvg();
    echo "narrow: rendered\n";
} catch (\Throwable $e) {
    echo "narrow: throws\n";
}

$svg = (new ParetoChart())->setSize(600, 300)->setBars($bars)->renderSvg();
echo "wide_renders: ", (strlen($svg) > 500 ? 'yes' : 'no'), "\n";

?>
--EXPECT--
narrow: throws
wide_renders: yes
