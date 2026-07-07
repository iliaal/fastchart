--TEST--
VectorChart honors setXAxisVisible(false): X gridlines and X labels are dropped
--EXTENSIONS--
fastchart
--FILE--
<?php

/* VectorChart hand-rolls its axis and drew the X gridlines + labels
 * unconditionally, ignoring setXAxisVisible(false). Gating the X loop on
 * x_axis_visible (matching the shared drawers) drops the vertical
 * gridlines and their labels while the arrows and Y grid remain. */

$vecs = [];
for ($x = 0; $x < 4; $x++)
    for ($y = 0; $y < 4; $y++)
        $vecs[] = ['x' => $x, 'y' => $y, 'dx' => 0.3, 'dy' => 0.3];

$vis = (new FastChart\VectorChart(400, 400))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setVectors($vecs)
    ->renderSvg();

$hidden = (new FastChart\VectorChart(400, 400))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setVectors($vecs)
    ->setXAxisVisible(false)
    ->renderSvg();

echo "hidden drops lines: ",
    substr_count($hidden, '<line') < substr_count($vis, '<line') ? "yes" : "no", "\n";
echo "hidden drops labels: ",
    substr_count($hidden, '<text') < substr_count($vis, '<text') ? "yes" : "no", "\n";
/* Arrows still drawn: 16 arrowhead polygons. */
echo "arrows survive: ", substr_count($hidden, '<polygon') >= 16 ? "yes" : "no", "\n";

?>
--EXPECT--
hidden drops lines: yes
hidden drops labels: yes
arrows survive: yes
