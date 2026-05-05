<?php
/* Three named series with legend, axis titles, and Catmull-Rom
 * smoothing. Markers highlight individual data points. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\LineChart(640, 380))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Quarterly revenue by region')
    ->setXAxisTitle('Quarter')
    ->setYAxisTitle('Revenue ($M)')
    ->setSeries([
        ['label' => 'Americas', 'data' => [42, 51, 47, 63, 71, 78, 85, 92]],
        ['label' => 'EMEA',     'data' => [28, 33, 35, 39, 45, 52, 58, 64]],
        ['label' => 'APAC',     'data' => [18, 22, 28, 34, 41, 48, 56, 67]],
    ])
    ->setCategoryLabels(['Q1\'24', 'Q2\'24', 'Q3\'24', 'Q4\'24',
                         'Q1\'25', 'Q2\'25', 'Q3\'25', 'Q4\'25'])
    ->setLineInterpolation(FastChart\Chart::INTERP_SMOOTH)
    ->setMarkerStyle(FastChart\Chart::MARKER_CIRCLE)
    ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT)
    ->renderToFile(__DIR__ . '/02_line_multi.png');
