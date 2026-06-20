--TEST--
Waterfall: rotated bar label anchors stay inside the SVG viewport
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_6192daeb: waterfall bar labels come from setBars(), not
 * setCategoryLabels(), so compute_layout couldn't measure them for the
 * rotated-label bottom margin and emitted label anchors below the canvas
 * (y far past the SVG height). The renderer now exposes the bar labels to the
 * margin measurement, and the shared categorical-axis drawer clamps a label
 * anchor that would still fall off a too-small canvas. (A label taller than
 * the canvas can't fully fit either way; this guards the anchor placement.) */

use FastChart\Waterfall;
use FastChart\Chart;

$label = str_repeat('LongStageName', 8);
$c = (new Waterfall())->setSize(320, 180)
    ->setSvgTextMode(Chart::SVG_TEXT_NATIVE)
    ->setXAxisLabelAngle(90)
    ->setBars([
        ['label' => $label, 'value' => 100],
        ['label' => $label, 'value' => 50],
        ['label' => $label, 'value' => -30],
    ]);
$svg = $c->renderSvg();

preg_match_all('/<text [^>]*\by="(-?\d+)"/', $svg, $m);
$max_y = max(array_map('intval', $m[1]));
echo "label_anchor_in_viewport: ", ($max_y <= 180 ? 'yes' : 'no'), "\n";

?>
--EXPECT--
label_anchor_in_viewport: yes
