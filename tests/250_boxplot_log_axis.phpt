--TEST--
BoxPlot: setYAxisScale(SCALE_LOG) is honored and rejects non-positive data
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_cc23a3ec: BoxPlot always built a linear range, silently ignoring
 * setYAxisScale(SCALE_LOG). It now branches to the log range helper. */

use FastChart\BoxPlot;
use FastChart\Chart;

$svg = (new BoxPlot())->setSize(400, 300)
    ->setSvgTextMode(Chart::SVG_TEXT_NATIVE)
    ->setYAxisScale(Chart::SCALE_LOG)
    ->setBoxes([['label' => 'a', 'min' => 1, 'q1' => 10, 'median' => 50, 'q3' => 100, 'max' => 1000]])
    ->renderSvg();
echo "log_renders: ", (strlen($svg) > 500 ? 'yes' : 'no'), "\n";
echo "log_decade_tick: ", (strpos($svg, '>100<') !== false ? 'yes' : 'no'), "\n";

try {
    (new BoxPlot())->setSize(400, 300)
        ->setYAxisScale(Chart::SCALE_LOG)
        ->setBoxes([['label' => 'a', 'min' => -5, 'q1' => 1, 'median' => 5, 'q3' => 10, 'max' => 20]])
        ->renderSvg();
    echo "nonpos: rendered\n";
} catch (\Throwable $e) {
    echo "nonpos: rejected\n";
}

?>
--EXPECT--
log_renders: yes
log_decade_tick: yes
nonpos: rejected
