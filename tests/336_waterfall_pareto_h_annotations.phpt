--TEST--
Waterfall and Pareto honor addHorizontalLine (value-axis threshold lines)
--EXTENSIONS--
fastchart
--INI--
asan.detect_leaks=0
--FILE--
<?php

/* addHorizontalLine is base-Chart API but the shared annotation drawer
 * was wired into only a handful of families. Waterfall and Pareto both
 * carry a real numeric value axis, so a threshold line is meaningful.
 * A line placed at a value must render as a dashed line spanning the
 * plot width, mapped onto the value axis (higher value -> higher on
 * screen). Before wiring, no annotation line was emitted at all. */

function ann_y(string $svg, string $hex): ?int {
    if (preg_match('/<line x1="(\d+)" y1="(\d+)" x2="(\d+)" y2="\2" stroke="' .
            $hex . '" stroke-dasharray=/', $svg, $m)) {
        return (int)$m[2];
    }
    return null;
}

/* Waterfall: cumulative range spans the data; two lines at 4 and 12. */
$wf = (new FastChart\Waterfall(500, 300))
    ->setBars([['label' => 'a', 'value' => 10], ['label' => 'b', 'value' => 5],
        ['label' => 'c', 'value' => -3]])
    ->addHorizontalLine(4.0, 'low', 0xFF0000)
    ->addHorizontalLine(12.0, 'high', 0x0000FF)
    ->renderSvg();
$wlo = ann_y($wf, '#FF0000');
$whi = ann_y($wf, '#0000FF');
echo 'waterfall lines present: ', ($wlo !== null && $whi !== null ? 'yes' : 'no'), "\n";
echo 'waterfall higher-value-higher: ', ($whi !== null && $wlo !== null && $whi < $wlo ? 'yes' : 'no'), "\n";

/* Pareto: left value axis is 0-based [0, y_axis_max]; two lines at 20/40. */
$pareto = (new FastChart\ParetoChart(500, 300))
    ->setBars([['label' => 'A', 'value' => 50], ['label' => 'B', 'value' => 30],
        ['label' => 'C', 'value' => 20]])
    ->addHorizontalLine(20.0, 'low', 0xFF0000)
    ->addHorizontalLine(40.0, 'high', 0x0000FF)
    ->renderSvg();
$plo = ann_y($pareto, '#FF0000');
$phi = ann_y($pareto, '#0000FF');
echo 'pareto lines present: ', ($plo !== null && $phi !== null ? 'yes' : 'no'), "\n";
echo 'pareto higher-value-higher: ', ($phi !== null && $plo !== null && $phi < $plo ? 'yes' : 'no'), "\n";

echo "done\n";
?>
--EXPECT--
waterfall lines present: yes
waterfall higher-value-higher: yes
pareto lines present: yes
pareto higher-value-higher: yes
done
