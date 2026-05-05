<?php
/* Plot bands + line annotations on a line chart: a target band
 * across the value axis plus a target line and a "deploy" marker
 * on the X axis. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\LineChart(720, 380))
    ->setFontPath($font)
    ->setTitle('Conversion rate over the deploy window')
    ->setYAxisTitle('Conversion (%)')
    ->setSeries([
        ['data' => [3.2, 3.4, 3.1, 3.3, 3.5, 4.1, 4.4, 4.2, 4.5, 4.7, 4.6, 4.8]],
    ])
    ->setCategoryLabels(['Wk1', 'Wk2', 'Wk3', 'Wk4', 'Wk5', 'Wk6',
                         'Wk7', 'Wk8', 'Wk9', 'Wk10', 'Wk11', 'Wk12'])
    ->addHorizontalBand(4.0, 5.0, 0x4FB286, 96, 'target')
    ->addHorizontalLine(4.5, 'goal', 0xE34A6F)
    ->addVerticalLine(5.0, 'deploy', 0x4F86C6)
    ->setMarkerStyle(FastChart\Chart::MARKER_CIRCLE)
    ->renderToFile(__DIR__ . '/11_plot_bands_annotations.png');
