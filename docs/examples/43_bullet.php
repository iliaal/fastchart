<?php
/* Bullet chart (Stephen Few): one horizontal performance bar
 * against qualitative bands and a target tick. Designed to
 * replace dashboard gauges with something compact and quickly
 * readable. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\BulletChart(560, 120))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Q3 revenue vs plan')
    ->setRange(0, 120)
    ->setBands([
        ['from' =>   0, 'to' =>  60, 'color' => 0xE5E7EB],
        ['from' =>  60, 'to' =>  90, 'color' => 0xCBD5E1],
        ['from' =>  90, 'to' => 120, 'color' => 0x94A3B8],
    ])
    ->setValue(78)
    ->setTarget(95)
    ->setValueFormat('%.0fM')
    ->renderToFile(__DIR__ . '/43_bullet.png');
