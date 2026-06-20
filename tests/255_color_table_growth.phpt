--TEST--
Color table: a chart with >512 distinct colors does not alias them to one handle
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_621462ad: after 512 distinct colors the fixed table aliased every new
 * color to the last handle, silently rendering wrong colors. The table is now
 * grown on demand. 600 per-point bubble colors must all survive. */

use FastChart\BubbleChart;

$pts = [];
for ($i = 0; $i < 600; $i++) {
    $pts[] = [$i, ($i * 7) % 100, 4, ($i * 9973 + 12345) & 0xFFFFFF];
}
$svg = (new BubbleChart())->setSize(1200, 900)->setPoints($pts)->renderSvg();

echo "renders: ", (strlen($svg) > 1000 ? 'yes' : 'no'), "\n";

preg_match_all('/(?:fill|stroke)="([^"]*)"/', $svg, $m);
$distinct = count(array_unique($m[1]));
echo "distinct_colors_gt_512: ", ($distinct > 512 ? 'yes' : 'no'), "\n";

?>
--EXPECT--
renders: yes
distinct_colors_gt_512: yes
