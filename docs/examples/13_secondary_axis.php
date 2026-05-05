<?php
/* Two value series rendered on independent Y axes: page views on the
 * left scale, revenue on the right (secondary). A series joins the
 * right axis by setting `'axis' => 'right'` in its dict. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\LineChart(720, 380))
    ->setFontPath($font)
    ->setTitle('Traffic vs revenue (independent scales)')
    ->setXAxisTitle('Week')
    ->setYAxisTitle('Page views')
    ->setSecondaryYAxis(true)
    ->setSecondaryYAxisTitle('Revenue ($K)')
    ->setSeries([
        ['label' => 'Page views',
         'data'  => [12000, 14500, 13200, 15800, 17400, 19200, 21000, 22500]],
        ['label' => 'Revenue', 'axis' => 'right',
         'data'  => [42, 48, 51, 58, 65, 72, 79, 88]],
    ])
    ->setCategoryLabels(['W1', 'W2', 'W3', 'W4', 'W5', 'W6', 'W7', 'W8'])
    ->setMarkerStyle(FastChart\Chart::MARKER_CIRCLE)
    ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT)
    ->renderToFile(__DIR__ . '/13_secondary_axis.png');
