<?php
/* Bar variants beyond the grouped/stacked basics:
 *   - setFloating(true): each entry is [min, max] and bars draw
 *     between those bounds (Gantt-style timelines, salary bands,
 *     temperature ranges).
 *   - setStackMode(STACK_LAYER): when stacked, layer translucent
 *     series in painter order at the same baseline instead of
 *     summing them. Useful for "is series A always larger than B?".
 *   - setStackMode(STACK_BESIDE): equivalent to setStacked(false)
 *     — keeps the chart in stacked-mode-API but renders side-by-side.
 */

require __DIR__ . '/_bootstrap.php';

(new FastChart\BarChart(640, 320))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Floating bars: forecast salary range by role')
    ->setFloating(true)   // must precede setSeries so the parser
                          // reads each entry as [min, max] not scalar
    ->setSeries([
        [80,  120],   // role A: $80k - $120k
        [110, 160],
        [140, 220],
        [180, 280],
        [220, 350],
    ])
    ->setCategoryLabels(['Junior', 'Mid', 'Senior', 'Staff', 'Principal'])
    ->setSeriesColors([0x4F86C6])
    ->setEdgeColor(0x222222)
    ->setYAxisTitle('Salary ($K)')
    ->renderToFile(__DIR__ . '/28a_bar_floating.png');

(new FastChart\BarChart(640, 320))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Layered stack: visual envelope of multiple series')
    ->setSeries([
        ['label' => 'Q1', 'data' => [42, 38, 51, 47, 55, 49]],
        ['label' => 'Q2', 'data' => [35, 41, 48, 50, 52, 54]],
        ['label' => 'Q3', 'data' => [28, 33, 39, 42, 47, 50]],
    ])
    ->setCategoryLabels(['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun'])
    ->setStacked(true)
    ->setStackMode(FastChart\Chart::STACK_LAYER)
    ->renderToFile(__DIR__ . '/28b_bar_layered.png');
