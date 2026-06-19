<?php
/* ChordDiagram: any-to-any flows between a small set of entities. Node
 * arcs are sized by total incident flow; each ribbon attaches to a
 * value-proportional slice of both endpoints and curves through the
 * centre. Translucent fills let overlapping chords blend. Bidirectional
 * travel between five European cities (each pair has a flow in both
 * directions), modeled after the amCharts chord-diagram demo. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\ChordDiagram(580, 580))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Travel between European cities')
    ->setNodes([
        ['label' => 'Berlin',    'color' => 0xCC4444],
        ['label' => 'Amsterdam', 'color' => 0xD79B00],
        ['label' => 'London',    'color' => 0x4477CC],
        ['label' => 'Paris',     'color' => 0x44AA88],
        ['label' => 'Madrid',    'color' => 0x9673A6],
    ])
    ->setLinks([
        ['from' => 0, 'to' => 1, 'value' => 14], ['from' => 1, 'to' => 0, 'value' => 42],
        ['from' => 0, 'to' => 2, 'value' => 22], ['from' => 2, 'to' => 0, 'value' => 31],
        ['from' => 0, 'to' => 3, 'value' => 18], ['from' => 3, 'to' => 0, 'value' => 25],
        ['from' => 1, 'to' => 2, 'value' => 35], ['from' => 2, 'to' => 1, 'value' => 19],
        ['from' => 1, 'to' => 3, 'value' => 12], ['from' => 3, 'to' => 1, 'value' => 28],
        ['from' => 2, 'to' => 3, 'value' => 40], ['from' => 3, 'to' => 2, 'value' => 33],
        ['from' => 2, 'to' => 4, 'value' => 16], ['from' => 4, 'to' => 2, 'value' => 24],
        ['from' => 3, 'to' => 4, 'value' => 30], ['from' => 4, 'to' => 3, 'value' => 21],
    ])
    ->setPadAngle(2.0)
    ->renderToFile(__DIR__ . '/58_chord_diagram.png');
