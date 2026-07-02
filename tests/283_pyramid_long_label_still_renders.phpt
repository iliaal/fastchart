--TEST--
PopulationPyramid: a very long category label does not blank the chart
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: the center gap was sized to the widest category label, and
 * when that exceeded the plot width the renderer returned before drawing
 * anything — a single long-but-valid label produced only the background.
 * The gap is now clamped to the available width so bars still render. */

$long = str_repeat('Category-', 30); /* ~270 chars */

$svg = (new FastChart\PopulationPyramid(400, 300))
    ->setCategories([$long, 'b', 'c'])
    ->setLeftSeries(['label' => 'L', 'data' => [10, 20, 30]])
    ->setRightSeries(['label' => 'R', 'data' => [12, 18, 25]])
    ->renderSvg();

/* Background is a single <rect>; drawn bars add several more. */
echo "rect_count_gt_1: ", (substr_count($svg, '<rect') > 1 ? "yes" : "no"), "\n";
echo "closed_svg: ", (str_contains($svg, '</svg>') ? "yes" : "no"), "\n";

?>
--EXPECT--
rect_count_gt_1: yes
closed_svg: yes
