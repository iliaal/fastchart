<?php
/* Every color / border / surface knob in one chart:
 *   - setBackgroundColor / setPlotBackgroundColor : chart vs plot rect
 *   - setTransparentBackground                    : alpha-clear chart bg
 *   - setAxisColor / setGridColor / setBorderColor / setTextColor
 *     setTitleColor / setAxisLabelColor / setAxisTitleColor
 *   - setBorderSides : bitmask of BORDER_TOP / RIGHT / BOTTOM / LEFT
 *   - setLineStyle   : LINE_SOLID / LINE_DASHED / LINE_DOTTED
 *   - setEdgeColor   : outline on filled shapes (bars, slices)
 *   - setBarWidth    : percent of slot width consumed by the bar (1..100)
 */

require __DIR__ . '/_bootstrap.php';

(new FastChart\LineChart(720, 360))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Custom palette across every painted surface')
    ->setBackgroundColor(0x1A1F2C)
    ->setPlotBackgroundColor(0x232A3D)
    ->setAxisColor(0x6E7894)
    ->setGridColor(0x363D52)
    ->setBorderColor(0x6E7894)
    ->setBorderSides(FastChart\Chart::BORDER_LEFT | FastChart\Chart::BORDER_BOTTOM)
    ->setTextColor(0xC9D1E0)
    ->setTitleColor(0xFFD166)
    ->setAxisLabelColor(0xA0AAC4)
    ->setAxisTitleColor(0xC9D1E0)
    ->setLineStyle(FastChart\Chart::LINE_DASHED)
    ->setSeriesColors([0x06D6A0, 0xFFD166, 0xEF476F])
    ->setSeries([
        ['label' => 'A', 'data' => [22, 35, 28, 41, 38, 47, 52]],
        ['label' => 'B', 'data' => [12, 18, 24, 22, 30, 33, 41]],
        ['label' => 'C', 'data' => [40, 36, 32, 30, 28, 24, 20]],
    ])
    ->setCategoryLabels(['Mon','Tue','Wed','Thu','Fri','Sat','Sun'])
    ->setXAxisTitle('Day')
    ->setYAxisTitle('Throughput')
    ->renderToFile(__DIR__ . '/23a_custom_palette.png');

/* Bar variant: edge color, narrowed bar width, transparent background
 * for compositing into a parent canvas / web page. */
(new FastChart\BarChart(720, 320))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Narrow bars with custom edge')
    ->setSeries([['data' => [22, 35, 28, 41, 38, 47, 52]]])
    ->setCategoryLabels(['Mon','Tue','Wed','Thu','Fri','Sat','Sun'])
    ->setBarWidth(50)
    ->setEdgeColor(0x222222)
    ->setSeriesColors([0x06D6A0])
    ->setTransparentBackground(true)
    ->renderToFile(__DIR__ . '/23b_narrow_bars.png');
