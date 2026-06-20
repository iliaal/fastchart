--TEST--
GaugeChart STYLE_SOLID: a value inside a reversed-bounds zone picks that zone's color
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_d0693f97: solid-style gauges tested the value against raw zone
 * from/to, so a reversed zone {from:80,to:50} never matched value 60. Zone
 * bounds are now normalized at setZones() time. */

use FastChart\GaugeChart;

$svg = (new GaugeChart())->setSize(300, 300)
    ->setStyle(GaugeChart::STYLE_SOLID)
    ->setRange(0, 100)
    ->setValue(60)
    ->setZones([['from' => 80, 'to' => 50, 'color' => 0xFF0000]])
    ->renderSvg();

echo "zone_color_used: ", (stripos($svg, 'ff0000') !== false ? 'yes' : 'no'), "\n";

?>
--EXPECT--
zone_color_used: yes
