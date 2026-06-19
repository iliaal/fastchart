<?php
/* Partition: a hierarchy as nested rectangular subdivisions. Each level
 * is a fixed band; a node's children split its span in proportion to
 * their subtree totals. ORIENT_VERTICAL is the icicle layout — the root
 * spans the top, descendants stack downward. Here, source size by
 * package. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\Partition(640, 420))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Source size by package (LOC)')
    ->setOrientation(FastChart\Partition::ORIENT_VERTICAL)
    ->setHierarchy([
        'label' => 'src',
        'children' => [
            ['label' => 'render', 'color' => 0x6C8EBF, 'children' => [
                ['label' => 'svg', 'value' => 34],
                ['label' => 'target', 'value' => 40],
                ['label' => 'text', 'value' => 12],
            ]],
            ['label' => 'charts', 'color' => 0x82B366, 'children' => [
                ['label' => 'bar', 'value' => 30],
                ['label' => 'stock', 'value' => 39],
                ['label' => 'pie', 'value' => 13],
            ]],
            ['label' => 'codec', 'color' => 0xD79B00, 'children' => [
                ['label' => 'png', 'value' => 16],
                ['label' => 'webp', 'value' => 8],
            ]],
        ],
    ])
    ->renderToFile(__DIR__ . '/68_partition.png');
