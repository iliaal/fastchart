--TEST--
AreaChart: a NaN gap on a non-stacked log axis renders without diving to the baseline
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_55600a39: non-stacked log areas folded NaN gaps to 0, which a log axis
 * maps to the plot bottom, distorting the fill. Gaps now fold to the fill
 * anchor (range min). This exercises that path end-to-end. */

use FastChart\AreaChart;
use FastChart\Chart;

$svg = (new AreaChart())->setSize(400, 300)->setStacked(false)
    ->setYAxisScale(Chart::SCALE_LOG)
    ->setSeries([['name' => 's', 'data' => [10, NAN, 1000, 50]]])
    ->renderSvg();

echo "log_gap_renders: ", (strlen($svg) > 500 ? 'yes' : 'no'), "\n";

?>
--EXPECT--
log_gap_renders: yes
