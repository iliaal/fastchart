<?php
/* Dendrogram: a hierarchy drawn as a node-link tree. Leaves are spaced
 * evenly, each parent is centred over its children, and depth runs down
 * the vertical axis. STYLE_ELBOW gives the classic right-angle clade
 * look. Here, a small taxonomy of the chart families themselves. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\Dendrogram(640, 480))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Chart taxonomy')
    ->setStyle(FastChart\Dendrogram::STYLE_ELBOW)
    ->setHierarchy([
        'label' => 'charts',
        'children' => [
            ['label' => 'cartesian', 'color' => 0x6C8EBF, 'children' => [
                ['label' => 'line'], ['label' => 'bar'], ['label' => 'area'],
            ]],
            ['label' => 'radial', 'color' => 0x82B366, 'children' => [
                ['label' => 'radar'], ['label' => 'polar'], ['label' => 'gauge'],
            ]],
            ['label' => 'hierarchy', 'color' => 0xD79B00, 'children' => [
                ['label' => 'treemap'], ['label' => 'sunburst'], ['label' => 'pack'],
            ]],
        ],
    ])
    ->renderToFile(__DIR__ . '/67_dendrogram.png');
