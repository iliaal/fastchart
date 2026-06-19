<?php
/* ArcDiagram: relationships in a 1-D ordering drawn as semicircular
 * arcs over a shared node baseline. Here, module dependencies in a
 * small codebase — arc thickness tracks how many symbols cross the
 * edge. SPLIT orientation routes forward edges above the baseline and
 * back-edges below, so cycles stand out. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\ArcDiagram(720, 320))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Module dependencies')
    ->setNodes([
        ['label' => 'cli',    'color' => 0x6C8EBF],
        ['label' => 'config'],
        ['label' => 'core',   'color' => 0xD79B00],
        ['label' => 'render'],
        ['label' => 'codec',  'color' => 0x9673A6],
        ['label' => 'io'],
    ])
    ->setLinks([
        ['from' => 0, 'to' => 1, 'value' => 3],
        ['from' => 0, 'to' => 2, 'value' => 6],
        ['from' => 2, 'to' => 3, 'value' => 5],
        ['from' => 3, 'to' => 4, 'value' => 4],
        ['from' => 4, 'to' => 5, 'value' => 2],
        ['from' => 3, 'to' => 1, 'value' => 1],   /* back-edge */
    ])
    ->setOrientation(FastChart\ArcDiagram::ORIENT_SPLIT)
    ->renderToFile(__DIR__ . '/57_arc_diagram.png');
