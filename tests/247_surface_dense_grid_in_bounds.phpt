--TEST--
SurfaceChart: a grid denser than the plot width stays inside the canvas
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_19e75288: integer cell width clamped to >=1px pushed the trailing cells
 * of a dense grid past the plot rect and off canvas. Cell boundaries are now
 * derived from the normalized plot span. */

use FastChart\SurfaceChart;

$rows = [];
for ($r = 0; $r < 3; $r++) {
    $row = [];
    for ($x = 0; $x < 1000; $x++) $row[] = $x % 10;
    $rows[] = $row;
}
$svg = (new SurfaceChart())->setSize(400, 300)->setGrid($rows)->renderSvg();

echo "renders: ", (strlen($svg) > 500 ? 'yes' : 'no'), "\n";

preg_match_all('/x="(\d+)"/', $svg, $m);
$max = 0;
foreach ($m[1] as $v) { if ((int)$v > $max) $max = (int)$v; }
echo "within_canvas: ", ($max <= 400 ? 'yes' : 'no'), "\n";

?>
--EXPECT--
renders: yes
within_canvas: yes
