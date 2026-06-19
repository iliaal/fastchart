<?php
/* NetworkChart: force-directed graph (Fruchterman-Reingold). Nodes
 * repel, links pull, and the layout settles into clusters. Deterministic
 * via the seed. Modeled after the amCharts force-directed-network demo:
 * a central hub linking four colour-coded clusters, each a lead plus its
 * members, with a few cross-cluster links. Node radius tracks degree. */

require __DIR__ . '/_bootstrap.php';

$blue = 0x4477CC; $green = 0x44AA88; $orange = 0xD79B00; $purple = 0x9673A6;

(new FastChart\NetworkChart(680, 560))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Team network')
    ->setNodes([
        ['label' => 'Core',  'color' => 0x555555],   /* 0 hub */
        ['label' => 'Web',   'color' => $blue],       /* 1 lead */
        ['label' => 'UI',    'color' => $blue],
        ['label' => 'UX',    'color' => $blue],
        ['label' => 'A11y',  'color' => $blue],
        ['label' => 'API',   'color' => $green],      /* 5 lead */
        ['label' => 'Auth',  'color' => $green],
        ['label' => 'Cache', 'color' => $green],
        ['label' => 'Queue', 'color' => $green],
        ['label' => 'Data',  'color' => $orange],     /* 9 lead */
        ['label' => 'ETL',   'color' => $orange],
        ['label' => 'Lake',  'color' => $orange],
        ['label' => 'BI',    'color' => $orange],
        ['label' => 'Ops',   'color' => $purple],     /* 13 lead */
        ['label' => 'CI',    'color' => $purple],
        ['label' => 'Infra', 'color' => $purple],
    ])
    ->setLinks([
        /* hub -> leads */
        ['from' => 0, 'to' => 1, 'value' => 5], ['from' => 0, 'to' => 5, 'value' => 5],
        ['from' => 0, 'to' => 9, 'value' => 5], ['from' => 0, 'to' => 13, 'value' => 5],
        /* Web cluster */
        ['from' => 1, 'to' => 2, 'value' => 3], ['from' => 1, 'to' => 3, 'value' => 3],
        ['from' => 1, 'to' => 4, 'value' => 2], ['from' => 2, 'to' => 3, 'value' => 1],
        /* API cluster */
        ['from' => 5, 'to' => 6, 'value' => 3], ['from' => 5, 'to' => 7, 'value' => 3],
        ['from' => 5, 'to' => 8, 'value' => 2],
        /* Data cluster */
        ['from' => 9, 'to' => 10, 'value' => 3], ['from' => 9, 'to' => 11, 'value' => 3],
        ['from' => 9, 'to' => 12, 'value' => 2],
        /* Ops cluster */
        ['from' => 13, 'to' => 14, 'value' => 3], ['from' => 13, 'to' => 15, 'value' => 3],
        /* cross-cluster links */
        ['from' => 5, 'to' => 1, 'value' => 2], ['from' => 9, 'to' => 5, 'value' => 2],
        ['from' => 13, 'to' => 5, 'value' => 2], ['from' => 8, 'to' => 10, 'value' => 1],
    ])
    ->setSeed(11)
    ->setIterations(500)
    ->renderToFile(__DIR__ . '/59_network.png');
