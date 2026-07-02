--TEST--
Treemap: more items than pixels never emits invalid (negative-size) rects
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* Regression: for a tiny plot rect, rounding could exhaust the remaining
 * width/height before every item was placed, leaving a cell with x1 < x0
 * (negative width) that reached fastchart_target_rect(). The paint loop
 * now skips any cell with rw <= 0 or rh <= 0. */

$items = [];
for ($i = 0; $i < 8; $i++) $items[] = ['label' => "I$i", 'value' => 1];

$svg = (new FastChart\Treemap(200, 200))
    ->setPlotRect(0, 0, 2, 2)   /* 3x3 px plot area, 8 equal items */
    ->setItems($items)
    ->renderSvg();

echo "valid_svg: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false ? "yes" : "no"),
    "\n";
echo "no_negative_dims: ", (preg_match('/(width|height)="-/', $svg) ? "BAD" : "yes"), "\n";

?>
--EXPECT--
valid_svg: yes
no_negative_dims: yes
