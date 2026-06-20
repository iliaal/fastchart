--TEST--
Waterfall: rotated long bar labels reserve bottom margin instead of overflowing the canvas
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_6192daeb: the bar labels live in setBars(), not setCategoryLabels(), so
 * compute_layout could not measure them for the rotated-label bottom margin and
 * fell back to the narrow "999999" probe — rotated long labels ran off canvas.
 * The renderer now exposes the bar labels to the margin measurement. */

use FastChart\Waterfall;
use FastChart\Chart;

$labels = ['Quarterly Opening Balance', 'Big Revenue Increase',
           'Operating Cost Reduction', 'Final Closing Total'];
$c = (new Waterfall())->setSize(300, 300)
    ->setSvgTextMode(Chart::SVG_TEXT_NATIVE)
    ->setXAxisLabelAngle(90)
    ->setBars([
        ['label' => $labels[0], 'value' => 100],
        ['label' => $labels[1], 'value' => 50],
        ['label' => $labels[2], 'value' => -30],
        ['label' => $labels[3], 'value' => 120, 'kind' => 'total'],
    ]);
$svg = $c->renderSvg();

preg_match_all('/<text [^>]*\by="(-?\d+)"/', $svg, $m);
$max_y = max(array_map('intval', $m[1]));
echo "labels_on_canvas: ", ($max_y <= 300 ? 'yes' : 'no'), "\n";

?>
--EXPECT--
labels_on_canvas: yes
