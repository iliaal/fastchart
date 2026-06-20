--TEST--
Horizontal BarChart: lollipop and dumbbell styles render bullets, not plain bars
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_5e867182: the horizontal renderer never branched on bar_style, so
 * lollipop/dumbbell silently fell back to plain rectangles. Both styles now
 * render with X/Y swapped like the vertical renderer. */

use FastChart\BarChart;

$loll = (new BarChart())->setSize(400, 300)
    ->setOrientation(BarChart::BAR_HORIZONTAL)
    ->setBarStyle(BarChart::BAR_STYLE_LOLLIPOP)
    ->setSeries([['name' => 's', 'data' => [10, 20, 15]]])
    ->renderSvg();
echo "h_lollipop_bullet: ",
    (strpos($loll, '<ellipse') !== false || strpos($loll, '<circle') !== false ? 'yes' : 'no'), "\n";

$dumb = (new BarChart())->setSize(400, 300)
    ->setOrientation(BarChart::BAR_HORIZONTAL)
    ->setBarStyle(BarChart::BAR_STYLE_DUMBBELL)
    ->setFloating(true)
    ->setSeries([['name' => 's', 'data' => [[10, 40], [20, 55]]]])
    ->renderSvg();
echo "h_dumbbell_bullet: ",
    (strpos($dumb, '<ellipse') !== false || strpos($dumb, '<circle') !== false ? 'yes' : 'no'), "\n";

?>
--EXPECT--
h_lollipop_bullet: yes
h_dumbbell_bullet: yes
