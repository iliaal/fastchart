<?php
/* Treemap: rectangle packing, area proportional to value. The
 * squarify algorithm picks aspect-ratio-friendly cell shapes so
 * even mixed-magnitude values stay readable. Common use: revenue
 * by product, traffic by source, error volume by service. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\Treemap(720, 440))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Q3 revenue by product line')
    ->setItems([
        ['label' => 'Cloud',     'value' => 4200],
        ['label' => 'Hardware',  'value' => 2900],
        ['label' => 'Services',  'value' => 2100],
        ['label' => 'Licenses',  'value' => 1500],
        ['label' => 'Training',  'value' => 900],
        ['label' => 'Support',   'value' => 700],
        ['label' => 'Marketing', 'value' => 400, 'color' => 0xE34A6F],
        ['label' => 'Other',     'value' => 300, 'color' => 0x9B9B9B],
    ])
    ->renderToFile(__DIR__ . '/32_treemap.png');
