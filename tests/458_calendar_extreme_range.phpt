--TEST--
CalendarHeatmap colors finite extreme values across an overflowing span
--EXTENSIONS--
fastchart
gd
--SKIPIF--
<?php
if (!function_exists('imagecreatefromstring')) die('skip gd required');
?>
--FILE--
<?php

use FastChart\CalendarHeatmap;

function ramp_colors(array $data): array {
    $png = (new CalendarHeatmap(500, 160))
        ->setData($data)
        ->setColorRamp(0xFF0000, 0x00FF00)
        ->renderPng();
    $im = imagecreatefromstring($png);
    $red = 0;
    $green = 0;
    for ($y = 0; $y < 160; $y++) {
        for ($x = 0; $x < 500; $x++) {
            $rgb = imagecolorat($im, $x, $y) & 0xFFFFFF;
            if ($rgb === 0xFF0000) $red++;
            if ($rgb === 0x00FF00) $green++;
        }
    }
    return [$red, $green];
}

foreach ([
    'max_span' => [-PHP_FLOAT_MAX, PHP_FLOAT_MAX],
    'proportional' => [-1.0, 1.0],
    'same_sign' => [-8e307, 8e307],
] as $name => [$low, $high]) {
    [$red, $green] = ramp_colors([
        '2026-01-01' => $low,
        '2026-01-02' => $high,
    ]);
    echo "$name: red=", ($red > 0 ? 'yes' : 'no'), " green=", ($green > 0 ? 'yes' : 'no'), "\n";
}

$base = 1e12;
$step = 0.0001220703125;
$close_data = [];
for ($i = 0; $i < 5; $i++) {
    $close_data[sprintf('2026-01-%02d', $i + 1)] = $base + $i * 2 * $step;
}
$chart = (new CalendarHeatmap(500, 160))
    ->setData($close_data)
    ->setColorRamp(0xFF0000, 0x00FF00)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
$svg = $chart->renderSvg();
$expected = [0xFF0000, 0xC03F00, 0x807F00, 0x40BF00, 0x00FF00];
$svg_ok = true;
foreach ($expected as $color) {
    if (!str_contains($svg, sprintf('fill="#%06X"', $color))) $svg_ok = false;
}
$im = imagecreatefromstring($chart->renderPng());
$png_ok = true;
foreach ($expected as $color) {
    $found = false;
    for ($y = 0; $y < 160 && !$found; $y++) {
        for ($x = 0; $x < 500; $x++) {
            if ((imagecolorat($im, $x, $y) & 0xFFFFFF) === $color) {
                $found = true;
                break;
            }
        }
    }
    if (!$found) $png_ok = false;
}
echo 'close_same_sign: svg=', ($svg_ok ? 'ok' : 'BAD'),
    " png=", ($png_ok ? 'ok' : 'BAD'), "\n";
?>
--EXPECT--
max_span: red=yes green=yes
proportional: red=yes green=yes
same_sign: red=yes green=yes
close_same_sign: svg=ok png=ok
