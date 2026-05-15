<?php
/* Pareto chart: descending bars + cumulative-percentage line. The
 * crossing point of the line with the 80% gridline tells you which
 * few categories explain most of the total (the 80/20 rule). */

require __DIR__ . '/_bootstrap.php';

(new FastChart\ParetoChart(680, 420))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Defect categories, last quarter')
    ->setBars([
        ['label' => 'Soldering',  'value' => 142],
        ['label' => 'Plating',    'value' =>  88],
        ['label' => 'Etching',    'value' =>  61],
        ['label' => 'Drilling',   'value' =>  34],
        ['label' => 'Cutting',    'value' =>  21],
        ['label' => 'Other',      'value' =>  12],
    ])
    ->setLineColor(0xE34A6F)
    ->setShowValues(true)
    ->renderToFile(__DIR__ . '/44_pareto.png');
