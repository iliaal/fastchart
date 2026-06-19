<?php
/* Radial (circular "race track") bar: setOrientation(BAR_RADIAL) draws
 * each category as a concentric ring whose bar is a thick arc swept
 * clockwise from 12 o'clock. The peak value reaches a near-full circle;
 * a faint track behind each bar shows the full scale. Multiple series
 * stack as concentric sub-bands. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\BarChart(520, 520))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Quarterly completion (%)')
    ->setCategoryLabels(['Design', 'Build', 'Test', 'Docs', 'Ship'])
    ->setOrientation(FastChart\BarChart::BAR_RADIAL)
    ->setSeries([
        ['label' => 'done', 'data' => [95, 80, 62, 48, 30]],
    ])
    ->renderToFile(__DIR__ . '/71_radial_bar.png');
