<?php
/* PopulationPyramid: two opposing series across shared category rows.
 * One side extends left of the central axis, the other right, on a
 * single shared value scale so the halves stay comparable. The classic
 * demographic age/sex breakdown. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\PopulationPyramid(680, 460))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Population by age and sex (%)')
    ->setCategories(['0-9', '10-19', '20-29', '30-39', '40-49', '50-59', '60-69', '70+'])
    ->setLeftSeries([
        'label' => 'Male',
        'color' => 0x3366CC,
        'data'  => [6.1, 6.4, 7.2, 7.0, 6.3, 5.4, 3.9, 2.6],
    ])
    ->setRightSeries([
        'label' => 'Female',
        'color' => 0xCC3366,
        'data'  => [5.8, 6.1, 6.9, 6.8, 6.2, 5.6, 4.3, 3.4],
    ])
    ->renderToFile(__DIR__ . '/60_population_pyramid.png');
