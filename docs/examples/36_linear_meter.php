<?php
/* Linear meter: bar-shaped gauge. Same zone / value / format
 * vocabulary as GaugeChart, rotated to a horizontal or vertical
 * bar — better fit for compact status / capacity readouts where a
 * round gauge would be too tall. */

require __DIR__ . '/_bootstrap.php';

/* Horizontal: SLA-style status bar with green / amber / red zones. */
(new FastChart\LinearMeter(680, 200))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('CPU utilisation')
    ->setRange(0, 100)
    ->setValue(72)
    ->setZones([
        ['from' => 0,  'to' => 60,  'color' => 0x4FB286],
        ['from' => 60, 'to' => 85,  'color' => 0xE8A33D],
        ['from' => 85, 'to' => 100, 'color' => 0xE34A6F],
    ])
    ->setValueFormat('%.0f%%')
    ->renderToFile(__DIR__ . '/36a_linear_meter_horizontal.png');

/* Vertical: thermometer-style readout, useful in dashboard
 * sidebars / dense status grids. */
(new FastChart\LinearMeter(280, 380))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Disk usage')
    ->setOrientation(FastChart\LinearMeter::METER_VERTICAL)
    ->setRange(0, 1024)
    ->setValue(680)
    ->setZones([
        ['from' => 0,    'to' => 700,  'color' => 0x4FB286],
        ['from' => 700,  'to' => 900,  'color' => 0xE8A33D],
        ['from' => 900,  'to' => 1024, 'color' => 0xE34A6F],
    ])
    ->setValueFormat('%.0f GB')
    ->renderToFile(__DIR__ . '/36b_linear_meter_vertical.png');
