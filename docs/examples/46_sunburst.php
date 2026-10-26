<?php
/* SunburstChart: nested ring donut. Each ring is one level of the
 * hierarchy; siblings divide their parent's span in proportion to
 * their values. These interior nodes omit value and sum their children. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\SunburstChart(520, 520))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Workload by team & project')
    ->setHierarchy([
        'label' => 'Eng',
        'children' => [
            ['label' => 'Backend', 'children' => [
                ['label' => 'API',      'value' => 18],
                ['label' => 'Workers',  'value' => 12],
                ['label' => 'DB',       'value' =>  9],
            ]],
            ['label' => 'Frontend', 'children' => [
                ['label' => 'Web',      'value' => 14],
                ['label' => 'Mobile',   'value' => 10],
            ]],
            ['label' => 'Infra', 'children' => [
                ['label' => 'CI',       'value' =>  7],
                ['label' => 'Observ.',  'value' =>  5],
                ['label' => 'Security', 'value' =>  4],
            ]],
        ],
    ])
    ->renderToFile(__DIR__ . '/46_sunburst.png');
