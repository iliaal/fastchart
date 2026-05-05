<?php
/* Every axis-tuning knob in one chart:
 *   - setYAxisRange(min, max, interval) — force fixed bounds + step
 *   - setYAxisScale(SCALE_LOG)         — log10 Y axis (positive data only)
 *   - setYAxisLabelFormat / setXAxisLabelFormat — printf format for ticks
 *   - setXAxisLabelAngle               — rotate X labels (0/45/90)
 *   - setXLabelStride                  — show every Nth label
 *   - setTickMode                      — TICK_LABELS / TICK_POINTS / TICK_BOTH / TICK_NONE
 *   - setXAxisVisible / setYAxisVisible — hide an axis entirely
 *   - setZeroShelf                     — heavier line at y=0 when data crosses zero
 */

require __DIR__ . '/_bootstrap.php';

(new FastChart\LineChart(720, 380))
    ->setFontPath($font)
    ->setTitle('Log-scale revenue with rotated date labels')
    ->setSeries([
        ['data' => [120, 240, 480, 920, 1840, 3650, 7200, 14400, 28000, 56000]],
    ])
    ->setCategoryLabels(['2016', '2017', '2018', '2019', '2020',
                         '2021', '2022', '2023', '2024', '2025'])
    ->setYAxisScale(FastChart\Chart::SCALE_LOG)
    ->setYAxisLabelFormat('$%.0f')
    ->setXAxisLabelAngle(45)
    ->setXLabelStride(1)
    ->setTickMode(FastChart\Chart::TICK_BOTH)
    ->setZeroShelf(true)
    ->setMarkerStyle(FastChart\Chart::MARKER_CIRCLE)
    ->renderToFile(__DIR__ . '/22a_axis_log.png');

/* Same data on a fixed-range linear axis with axis lines hidden,
 * forcing visual focus on the line itself. */
(new FastChart\LineChart(720, 380))
    ->setFontPath($font)
    ->setTitle('Linear with fixed range, axis lines off')
    ->setSeries([
        ['data' => [22, 35, 28, 41, 38, 47, 52, 49, 58, 65]],
    ])
    ->setCategoryLabels(range(2016, 2025))
    ->setYAxisRange(0, 100, 25)
    ->setXAxisVisible(false)
    ->setYAxisVisible(false)
    ->setLineInterpolation(FastChart\Chart::INTERP_SMOOTH)
    ->renderToFile(__DIR__ . '/22b_axis_minimal.png');
