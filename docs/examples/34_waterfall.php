<?php
/* Waterfall: rising / falling / total bar semantics with auto
 * color-by-sign. Each delta bar starts at the running cumulative
 * and runs to cumulative + value; total bars render absolute from
 * zero and reset the cumulative. The ribbon connectors between
 * bars trace the running total. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\Waterfall(720, 400))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Q3 income statement ($M)')
    ->setBars([
        ['label' => 'Revenue', 'value' => 1200, 'kind' => 'total'],
        ['label' => 'COGS',    'value' => -480],
        ['label' => 'Margin',  'value' => 720,  'kind' => 'total'],
        ['label' => 'R&D',     'value' => -180],
        ['label' => 'S&M',     'value' => -220],
        ['label' => 'G&A',     'value' => -90],
        ['label' => 'OpInc',   'value' => 230,  'kind' => 'total'],
    ])
    ->renderToFile(__DIR__ . '/34_waterfall.png');
