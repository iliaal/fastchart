--TEST--
BubbleChart: an extreme finite X coordinate maps through the guarded helper (no overflow cast)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_c23903ed: the X coordinate was mapped with an inline unclamped fraction
 * and cast directly to int, so PHP_FLOAT_MAX overflowed the cast (UB). It now
 * routes through fastchart_x_to_pixel like the Y path. */

use FastChart\BubbleChart;

$svg = (new BubbleChart())->setSize(400, 300)
    ->setPoints([[PHP_FLOAT_MAX, 1, 5], [1, 2, 5]])
    ->renderSvg();
echo "renders: ", (strlen($svg) > 500 ? 'yes' : 'no'), "\n";

?>
--EXPECT--
renders: yes
