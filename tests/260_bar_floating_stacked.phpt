--TEST--
BarChart: floating bars ignore stacked / STACK_LAYER (no negative-width rects)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_15c1121f: setFloating + setStacked (or STACK_LAYER) collapsed the
 * sub-slot count to 1 while the floating branch still offset each series
 * by s * sub_w, pushing every series after the first past the slot edge
 * into negative-width rects (invalid SVG, series invisible). Floating now
 * forces side-by-side rendering in both orientations. */

use FastChart\BarChart;
use FastChart\Chart;

function chart(bool $stacked, int $stack_mode = 0, int $orient = 0): string {
    $c = (new BarChart(600, 400))
        ->setFloating(true)
        ->setSeries([
            ['data' => [[10, 40], [30, 80]]],
            ['data' => [[20, 60], [50, 90]]],
        ])
        ->setCategoryLabels(['A', 'B']);
    if ($stacked) $c->setStacked(true);
    if ($stack_mode) $c->setStackMode($stack_mode);
    if ($orient) $c->setOrientation($orient);
    return $c->renderSvg();
}

$base = chart(false);
echo "baseline negative dims: ",
    (preg_match('/(width|height)="-/', $base) ? 'FOUND' : 'none'), "\n";

foreach ([
    'stacked'            => chart(true),
    'stack_layer'        => chart(false, Chart::STACK_LAYER),
    'stacked_horizontal' => chart(true, 0, BarChart::BAR_HORIZONTAL),
] as $name => $svg) {
    echo "$name negative dims: ",
        (preg_match('/(width|height)="-/', $svg) ? 'FOUND' : 'none'), "\n";
    echo "$name matches baseline rect count: ",
        (substr_count($svg, '<rect') === substr_count(
            $name === 'stacked_horizontal'
                ? chart(false, 0, BarChart::BAR_HORIZONTAL) : $base, '<rect')
         ? 'yes' : 'NO'), "\n";
}

?>
--EXPECT--
baseline negative dims: none
stacked negative dims: none
stacked matches baseline rect count: yes
stack_layer negative dims: none
stack_layer matches baseline rect count: yes
stacked_horizontal negative dims: none
stacked_horizontal matches baseline rect count: yes
