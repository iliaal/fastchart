<?php
/* Multi-line title and labels: any text passed to setTitle() /
 * setCategoryLabels() / etc. that contains a "\n" is split, laid
 * out per-line at the per-line advance, and the layout reservation
 * grows accordingly so the plot rect is pushed down by the right
 * amount. The single-line path stays as the fast common case. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\LineChart(720, 420))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle("Quarterly revenue\n(USD millions, FY 2025)")
    ->setSeries([
        ['label' => 'AMER', 'data' => [120, 145, 168, 192]],
        ['label' => 'EMEA', 'data' => [ 90, 102, 118, 135]],
        ['label' => 'APAC', 'data' => [ 60,  75,  88, 105]],
    ])
    ->setCategoryLabels(['Q1', 'Q2', 'Q3', 'Q4'])
    ->renderToFile(__DIR__ . '/39_multiline_text.png');
