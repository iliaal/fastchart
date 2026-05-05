<?php
/* Half-circle gauge with three colored zones and a needle pointing
 * at the current value. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\GaugeChart(480, 320))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Server CPU utilization')
    ->setRange(0, 100)
    ->setValue(72)
    ->setZones([
        ['from' => 0,  'to' => 50,  'color' => 0x4FB286],
        ['from' => 50, 'to' => 80,  'color' => 0xF6AE2D],
        ['from' => 80, 'to' => 100, 'color' => 0xE34A6F],
    ])
    ->setValueFormat('%.0f%%')
    ->renderToFile(__DIR__ . '/10_gauge.png');
