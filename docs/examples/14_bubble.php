<?php
/* BubbleChart: each point is [x, y, size]. Bubble area scales with
 * the third dimension so visual weight matches magnitude. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\BubbleChart(640, 400))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Project size vs delivery risk')
    ->setXAxisTitle('Estimated effort (story points)')
    ->setYAxisTitle('Risk score')
    ->setPoints([
        [8,  2.5, 5],
        [13, 1.8, 8],
        [21, 3.2, 12],
        [34, 5.5, 18],
        [55, 7.2, 25],
        [89, 8.9, 40],
        [21, 4.0, 9],
        [34, 6.3, 15],
        [13, 2.8, 4],
    ])
    ->renderToFile(__DIR__ . '/14_bubble.png');
