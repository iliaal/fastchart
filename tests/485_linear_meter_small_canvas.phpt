--TEST--
LinearMeter rejects collapsed default plots and honors explicit plot bounds
--EXTENSIONS--
fastchart
--FILE--
<?php
use FastChart\LinearMeter;

foreach ([
    'horizontal inverted' => [100, 160, LinearMeter::METER_HORIZONTAL],
    'horizontal collapsed' => [120, 160, LinearMeter::METER_HORIZONTAL],
    'vertical inverted' => [160, 60, LinearMeter::METER_VERTICAL],
    'vertical collapsed' => [160, 72, LinearMeter::METER_VERTICAL],
] as $name => [$width, $height, $orientation]) {
    $chart = (new LinearMeter($width, $height))
        ->setOrientation($orientation)->setRange(0, 100)->setValue(50)
        ->setZones([['from' => 0, 'to' => 100, 'color' => 0x123456]]);
    try {
        $chart->renderSvg();
        echo "$name: accepted invalid plot\n";
    } catch (ValueError $e) {
        echo "$name: ", $e->getMessage(), "\n";
    }
    // An explicit plot fits even when the default margins do not.
    $chart->setPlotRect(10, 10, $width - 10, $height - 10);
    $svg = $chart->renderSvg();
    echo "$name override: ",
        str_contains($svg, 'fill="#123456"') &&
        !preg_match('/(?:width|height)="-/', $svg) &&
        $svg === $chart->renderSvg() ? 'ok' : 'FAIL', "\n";
}

// The first non-collapsed default plots still render in both orientations.
foreach ([
    [121, 160, LinearMeter::METER_HORIZONTAL],
    [160, 73, LinearMeter::METER_VERTICAL],
] as [$width, $height, $orientation]) {
    $chart = (new LinearMeter($width, $height))->setOrientation($orientation);
    echo "boundary $width x $height: ",
        str_contains($chart->renderSvg(), '<polygon') ? 'ok' : 'FAIL', "\n";
}

// Explicit rectangles use inclusive endpoints, so one-pixel plots are valid.
foreach ([LinearMeter::METER_HORIZONTAL, LinearMeter::METER_VERTICAL] as $orientation) {
    $chart = (new LinearMeter(100, 100))->setOrientation($orientation)
        ->setPlotRect(25, 25, 25, 25);
    $svg = $chart->renderSvg();
    echo "one-pixel $orientation: ",
        str_contains($svg, '<rect x="25" y="25" width="1" height="1"') ? 'ok' : 'FAIL', "\n";
}

// A failed render must leave the object usable after resizing.
$chart = new LinearMeter(100, 160);
try { $chart->renderSvg(); } catch (ValueError $e) {}
$chart->setSize(320, 160);
echo "resized: ", str_contains($chart->renderSvg(), '<polygon') ? 'ok' : 'FAIL', "\n";
?>
--EXPECT--
horizontal inverted: FastChart\LinearMeter::draw() canvas is too small for meter margins
horizontal inverted override: ok
horizontal collapsed: FastChart\LinearMeter::draw() canvas is too small for meter margins
horizontal collapsed override: ok
vertical inverted: FastChart\LinearMeter::draw() canvas is too small for meter margins
vertical inverted override: ok
vertical collapsed: FastChart\LinearMeter::draw() canvas is too small for meter margins
vertical collapsed override: ok
boundary 121 x 160: ok
boundary 160 x 73: ok
one-pixel 0: ok
one-pixel 1: ok
resized: ok
