--TEST--
Horizontal BarChart: an extreme category-annotation position is bounded before the int cast
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_b94919b8: the horizontal-bar category annotation cast v to int before
 * the bounds check, so a finite-but-huge addVerticalLine() position was UB.
 * It is now range-guarded (skipped) before the cast, mirroring the vertical
 * categorical mapper. */

use FastChart\BarChart;

$c = (new BarChart())->setSize(400, 300)
    ->setOrientation(BarChart::BAR_HORIZONTAL)
    ->setSeries([['name' => 's', 'data' => [10, 20, 30]]]);
$c->addVerticalLine(1e300, 'x');   /* out-of-range category index */

$svg = $c->renderSvg();
echo "renders: ", (strlen($svg) > 500 ? 'yes' : 'no'), "\n";

?>
--EXPECT--
renders: yes
