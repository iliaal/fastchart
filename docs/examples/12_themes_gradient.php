<?php
/* Same data drawn three ways: light theme, dark theme, and a
 * gradient-fill bar chart with a drop shadow on the bars — to show
 * how the presentation knobs compose. Three charts on three files. */

require __DIR__ . '/_bootstrap.php';

$data = [['data' => [22, 35, 28, 41, 38, 47, 52]]];
$labels = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'];

(new FastChart\BarChart(420, 260))
    ->setFontPath($font)
    ->setTitle('Light theme')
    ->setSeries($data)
    ->setCategoryLabels($labels)
    ->setTheme(FastChart\Chart::THEME_LIGHT)
    ->renderToFile(__DIR__ . '/12a_theme_light.png');

(new FastChart\BarChart(420, 260))
    ->setFontPath($font)
    ->setTitle('Dark theme')
    ->setSeries($data)
    ->setCategoryLabels($labels)
    ->setTheme(FastChart\Chart::THEME_DARK)
    ->renderToFile(__DIR__ . '/12b_theme_dark.png');

(new FastChart\BarChart(420, 260))
    ->setFontPath($font)
    ->setTitle('Gradient + shadow')
    ->setSeries($data)
    ->setCategoryLabels($labels)
    ->setGradientFill(0x4F86C6, 0xE34A6F, FastChart\Chart::GRADIENT_VERTICAL)
    ->setDropShadow(3, 3, 0x000000)
    ->setShadowAlpha(80)
    ->renderToFile(__DIR__ . '/12c_gradient.png');
