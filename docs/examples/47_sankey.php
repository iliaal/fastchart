<?php
/* SankeyChart: bipartite / multi-layer flow with bezier ribbons.
 * Node positions are computed from a topological pass; ribbon
 * widths are proportional to flow value. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\SankeyChart(760, 400))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Acquisition → activation funnel')
    ->setNodes([
        ['label' => 'Search'],
        ['label' => 'Social'],
        ['label' => 'Referral'],
        ['label' => 'Landing'],
        ['label' => 'Signup'],
        ['label' => 'Active'],
        ['label' => 'Churned'],
    ])
    ->setLinks([
        ['from' => 0, 'to' => 3, 'value' => 60],
        ['from' => 1, 'to' => 3, 'value' => 25],
        ['from' => 2, 'to' => 3, 'value' => 15],
        ['from' => 3, 'to' => 4, 'value' => 70],
        ['from' => 3, 'to' => 6, 'value' => 30],
        ['from' => 4, 'to' => 5, 'value' => 55],
        ['from' => 4, 'to' => 6, 'value' => 15],
    ])
    ->renderToFile(__DIR__ . '/47_sankey.png');
