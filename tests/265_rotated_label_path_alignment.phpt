--TEST--
Rotated path-mode axis labels pivot at the same anchor as native <text> labels
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_10ae5c60: fc_svg_emit_text_as_path folded the right-align shift
 * into the pre-rotation translate, so rotated path-mode labels pivoted
 * at (x+shift, y) and drifted left of their tick by up to the label
 * width (wide labels landed off-canvas). The shift is now applied along
 * the rotated baseline, matching the native text-anchor semantics. */

use FastChart\BarChart;
use FastChart\Chart;

function svg(int $angle, int $mode): string {
    return (new BarChart(400, 300))
        ->setSeries([5, 9, 3])
        ->setCategoryLabels(['WIDE-LABEL-AAAA', 'B', 'WIDE-LABEL-CCCC'])
        ->setXAxisLabelAngle($angle)
        ->setSvgTextMode($mode)
        ->renderSvg();
}

foreach ([45, 90] as $angle) {
    $native = svg($angle, Chart::SVG_TEXT_NATIVE);
    $paths  = svg($angle, Chart::SVG_TEXT_PATHS);

    preg_match_all(
        '/text-anchor="end" transform="rotate\([-0-9.]+ ([-0-9.]+) ([-0-9.]+)\)"/',
        $native, $mn, PREG_SET_ORDER);
    preg_match_all(
        '/<g transform="translate\(([-0-9.]+) ([-0-9.]+)\) rotate\([-0-9.]+\)( translate\([-0-9.]+ 0\))?"/',
        $paths, $mp, PREG_SET_ORDER);

    echo "angle $angle labels: ", count($mn), " native / ", count($mp), " paths\n";
    $match = count($mn) === 3 && count($mp) === 3;
    for ($i = 0; $match && $i < 3; $i++) {
        if ((float)$mn[$i][1] !== (float)$mp[$i][1]
            || (float)$mn[$i][2] !== (float)$mp[$i][2]) {
            $match = false;
        }
    }
    echo "angle $angle pivots match native anchors: ", $match ? 'yes' : 'NO', "\n";
    /* Wide labels are right-aligned, so their shift must ride AFTER the
     * rotate as a second translate. */
    echo "angle $angle wide labels shift post-rotation: ",
        (!empty($mp[0][3]) && !empty($mp[2][3]) ? 'yes' : 'NO'), "\n";
}

?>
--EXPECT--
angle 45 labels: 3 native / 3 paths
angle 45 pivots match native anchors: yes
angle 45 wide labels shift post-rotation: yes
angle 90 labels: 3 native / 3 paths
angle 90 pivots match native anchors: yes
angle 90 wide labels shift post-rotation: yes
