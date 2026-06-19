<?php
/* CirclePacking: a hierarchy drawn as nested circles. Leaf area tracks
 * value; each parent is the enclosing circle of its packed children.
 * A compact alternative to a treemap for part-of-whole hierarchies.
 * Here, lines of code by package and module. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\CirclePacking(560, 560))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Source size by package (LOC)')
    ->setHierarchy([
        'label' => 'src',
        'children' => [
            ['label' => 'render', 'color' => 0x6C8EBF, 'children' => [
                ['label' => 'svg',   'value' => 34],
                ['label' => 'target','value' => 40],
                ['label' => 'text',  'value' => 12],
            ]],
            ['label' => 'charts', 'color' => 0x82B366, 'children' => [
                ['label' => 'line',  'value' => 9],
                ['label' => 'bar',   'value' => 30],
                ['label' => 'stock', 'value' => 39],
                ['label' => 'pie',   'value' => 13],
            ]],
            ['label' => 'codec', 'color' => 0xD79B00, 'children' => [
                ['label' => 'png',  'value' => 16],
                ['label' => 'webp', 'value' => 8],
            ]],
        ],
    ])
    ->renderToFile(__DIR__ . '/62_circle_packing.png');
