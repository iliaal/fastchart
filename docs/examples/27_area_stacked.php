<?php
/* AreaChart in two flavors:
 *   - stacked: each series sits on top of the cumulative total below
 *   - overlay (setStacked false): series overlap with translucent
 *     fills controlled by setFillOpacity (0 = transparent, 127 = solid)
 */

require __DIR__ . '/_bootstrap.php';

$labels = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug'];
$data = [
    ['label' => 'Free',  'data' => [320, 380, 410, 470, 520, 590, 640, 700]],
    ['label' => 'Pro',   'data' => [120, 150, 180, 210, 250, 290, 340, 400]],
    ['label' => 'Team',  'data' => [40,  55,  70,  95,  130, 170, 210, 260]],
];

(new FastChart\AreaChart(640, 320))
    ->setFontPath($font)
    ->setTitle('Active users by tier (stacked)')
    ->setSeries($data)
    ->setCategoryLabels($labels)
    ->setStacked(true)
    ->setFillOpacity(110)
    ->renderToFile(__DIR__ . '/27a_area_stacked.png');

(new FastChart\AreaChart(640, 320))
    ->setFontPath($font)
    ->setTitle('Active users by tier (overlay)')
    ->setSeries($data)
    ->setCategoryLabels($labels)
    ->setStacked(false)
    ->setFillOpacity(60)
    ->renderToFile(__DIR__ . '/27b_area_overlay.png');
