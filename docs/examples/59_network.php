<?php
/* NetworkChart: force-directed graph. Nodes repel, links pull, and the
 * layout settles into clusters. Deterministic — the seed fixes the
 * initial placement, so the same data always renders identically.
 * Here, two collaborating teams with a bridging lead. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\NetworkChart(640, 520))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Team collaboration graph')
    ->setNodes([
        ['label' => 'Ana',  'color' => 0x6C8EBF],
        ['label' => 'Ben',  'color' => 0x6C8EBF],
        ['label' => 'Cy',   'color' => 0x6C8EBF],
        ['label' => 'Dee',  'color' => 0x6C8EBF],
        ['label' => 'Eve',  'color' => 0xD79B00],   /* bridge */
        ['label' => 'Fin',  'color' => 0x82B366],
        ['label' => 'Gus',  'color' => 0x82B366],
        ['label' => 'Hana', 'color' => 0x82B366],
    ])
    ->setLinks([
        ['from' => 0, 'to' => 1, 'value' => 3],
        ['from' => 0, 'to' => 2, 'value' => 2],
        ['from' => 1, 'to' => 3, 'value' => 2],
        ['from' => 2, 'to' => 3, 'value' => 1],
        ['from' => 3, 'to' => 4, 'value' => 4],   /* bridge */
        ['from' => 4, 'to' => 5, 'value' => 4],
        ['from' => 5, 'to' => 6, 'value' => 2],
        ['from' => 5, 'to' => 7, 'value' => 3],
        ['from' => 6, 'to' => 7, 'value' => 2],
    ])
    ->setSeed(7)
    ->setIterations(400)
    ->renderToFile(__DIR__ . '/59_network.png');
