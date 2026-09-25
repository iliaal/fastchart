--TEST--
ScatterChart trend fitting remains aligned for constant extreme Y values
--EXTENSIONS--
fastchart
gd
--SKIPIF--
<?php
if (!function_exists('imagecreatefromstring')) die('skip gd required');
?>
--FILE--
<?php

function trend_is_interior(string $png): string {
    $im = imagecreatefromstring($png);
    $red = 0;
    $max_y = 0;
    for ($y = 0; $y < 400; $y++) {
        for ($x = 0; $x < 500; $x++) {
            if ((imagecolorat($im, $x, $y) & 0xFFFFFF) === 0xFF0000) {
                $red++;
                $max_y = max($max_y, $y);
            }
        }
    }
    return $red > 500 && $max_y < 260 ? 'ok' : "BAD(red=$red,max_y=$max_y)";
}

function points(int $count): array {
    $out = [];
    for ($i = 0; $i < $count; $i++) $out[] = [$i, 1e307];
    return $out;
}

function red_line_ys(string $svg): array {
    preg_match_all('/<line x1="[-0-9.]+" y1="([-0-9.]+)" x2="[-0-9.]+" y2="[-0-9.]+" stroke="#FF0000" stroke-width="2"/', $svg, $m);
    return array_map('floatval', $m[1]);
}

function plot_bottom(string $svg): float {
    preg_match_all('/<rect x="[0-9.-]+" y="([0-9.-]+)" width="[0-9.-]+" height="([0-9.-]+)" fill="#FFFFFF"/', $svg, $m);
    return (float)$m[1][1] + (float)$m[2][1] - 1.0;
}

function expected_y(float $value, float $min, float $max, float $bottom): int {
    return (int)($bottom - (int)((($value - $min) / ($max - $min)) * 355.0 + 0.5));
}

foreach ([17, 18, 20] as $count) {
    foreach ([1, 2] as $degree) {
        $png = (new FastChart\ScatterChart(500, 400))
            ->setPoints(points($count))
            ->setTrendLine(true, 0xFF0000, $degree)
            ->renderPng();
        echo "n{$count}_deg{$degree}: ", trend_is_interior($png), "\n";
    }
}

$base = 1e9;
$step = 0.00000011920928955078125;
$low_x = [[$base, 1.0], [$base, -2.0], [$base + 3 * $step, -1.0], [$base + 7 * $step, 3.0]];
$low_x_svg = (new FastChart\ScatterChart(500, 400))
    ->setPoints($low_x)
    ->setYAxisRange(-5, 5)
    ->setLegendPosition(FastChart\Chart::LEGEND_NONE)
    ->setTrendLine(true, 0xFF0000, 1)
    ->renderSvg();
$low_x_ys = red_line_ys($low_x_svg);
echo 'low_order_x: ',
    (count($low_x_ys) === 200 && abs($low_x_ys[0] - 218) <= 1 && abs($low_x_ys[199] - 102) <= 1 ? 'ok' : 'BAD'), "\n";

$residual_y = [
    [-1, 1e308], [-1, -1e308], [1, -1e308],
    [1, 1e308], [0, 1e-100], [0, 1e-100],
];
foreach ([1] as $degree) {
    $chart = (new FastChart\ScatterChart(500, 400))
        ->setPoints($residual_y)
        ->setYAxisRange(-1e-100, 1e-100)
        ->setLegendPosition(FastChart\Chart::LEGEND_NONE)
        ->setTrendLine(true, 0xFF0000, $degree);
    $svg = $chart->renderSvg();
    $want = expected_y(1e-100 / 3, -1e-100, 1e-100, plot_bottom($svg));
    $ys = red_line_ys($svg);
    $svg_ok = count($ys) === 200 && abs($ys[0] - $want) <= 1;

    $im = imagecreatefromstring($chart->renderPng());
    $red = 0;
    for ($y = $want - 1; $y <= $want + 1; $y++) {
        for ($x = 0; $x < 500; $x++) {
            if ((imagecolorat($im, $x, $y) & 0xFFFFFF) === 0xFF0000) $red++;
        }
    }
    echo "residual_y_deg{$degree}: svg=", ($svg_ok ? 'ok' : 'BAD'),
        " png=", ($red > 200 ? 'ok' : 'BAD'), "\n";
}
?>
--EXPECT--
n17_deg1: ok
n17_deg2: ok
n18_deg1: ok
n18_deg2: ok
n20_deg1: ok
n20_deg2: ok
low_order_x: ok
residual_y_deg1: svg=ok png=ok
