<?php
/* ChordDiagram: any-to-any flows between a small set of entities. Node
 * arcs are sized by total incident flow; each ribbon attaches to a
 * value-proportional slice of both endpoints and curves through the
 * centre. Here, weekday commuter flow between four city districts. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\ChordDiagram(560, 560))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Commuter flow between districts')
    ->setNodes([
        ['label' => 'Centre', 'color' => 0xCC4444],
        ['label' => 'North',  'color' => 0x4477CC],
        ['label' => 'East',   'color' => 0x44AA88],
        ['label' => 'West',   'color' => 0xD79B00],
    ])
    ->setLinks([
        ['from' => 0, 'to' => 1, 'value' => 18],
        ['from' => 0, 'to' => 2, 'value' => 12],
        ['from' => 0, 'to' => 3, 'value' => 15],
        ['from' => 1, 'to' => 2, 'value' => 6],
        ['from' => 2, 'to' => 3, 'value' => 9],
        ['from' => 1, 'to' => 3, 'value' => 4],
    ])
    ->setPadAngle(3.0)
    ->renderToFile(__DIR__ . '/58_chord_diagram.png');
