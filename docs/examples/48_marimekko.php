<?php
/* MarimekkoChart (Mekko): variable-width stacked columns. Each
 * column's width is proportional to its category total; segment
 * heights within each column proportional to component values.
 * Reads as percent-of-category AND percent-of-total in one shape. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\MarimekkoChart(700, 480))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Revenue mix by region & product')
    ->setColumns([
        ['label' => 'NA',  'segments' => [
            ['label' => 'Hardware', 'value' => 80, 'color' => 0x2563EB],
            ['label' => 'Services', 'value' => 50, 'color' => 0x10B981],
            ['label' => 'Cloud',    'value' => 70, 'color' => 0xF59E0B],
        ]],
        ['label' => 'EMEA', 'segments' => [
            ['label' => 'Hardware', 'value' => 45, 'color' => 0x2563EB],
            ['label' => 'Services', 'value' => 35, 'color' => 0x10B981],
            ['label' => 'Cloud',    'value' => 25, 'color' => 0xF59E0B],
        ]],
        ['label' => 'APAC', 'segments' => [
            ['label' => 'Hardware', 'value' => 30, 'color' => 0x2563EB],
            ['label' => 'Services', 'value' => 15, 'color' => 0x10B981],
            ['label' => 'Cloud',    'value' => 40, 'color' => 0xF59E0B],
        ]],
        ['label' => 'LATAM','segments' => [
            ['label' => 'Hardware', 'value' => 12, 'color' => 0x2563EB],
            ['label' => 'Services', 'value' =>  8, 'color' => 0x10B981],
        ]],
    ])
    ->renderToFile(__DIR__ . '/48_marimekko.png');
