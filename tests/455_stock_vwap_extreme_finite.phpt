--TEST--
StockChart VWAP handles finite extreme prices and volumes
--EXTENSIONS--
fastchart
gd
--SKIPIF--
<?php
if (!function_exists('imagecreatefromstring')) die('skip gd required');
?>
--FILE--
<?php

function vwap_ys(string $svg): array {
    preg_match_all('/<line x1="([-0-9.]+)" y1="([-0-9.]+)" x2="([-0-9.]+)" y2="([-0-9.]+)" stroke="#0000FF" stroke-width="2"/', $svg, $m);
    return array_map('floatval', $m[2]);
}

function plot_geometry(string $svg): array {
    preg_match_all('/<rect x="[0-9.-]+" y="([0-9.-]+)" width="[0-9.-]+" height="([0-9.-]+)" fill="#FFFFFF"/', $svg, $m);
    $height = (float)$m[2][1] - 1.0;
    return [(float)$m[1][1] + $height, $height];
}
function expected_y(float $value, float $min, float $max, array $plot): int {
    [$bottom, $height] = $plot;
    $frac = ($value - $min) / ($max - $min);
    return (int)($bottom - (int)($frac * $height + 0.5));
}

function check_vwap(string $name, array $rows, float $value, float $min, float $max): void {
    $chart = (new FastChart\StockChart(600, 400))
        ->setOhlcv($rows)
        ->setYAxisRange($min, $max)
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->addVWAP(0x0000FF);
    $svg = $chart->renderSvg();
    $ys = vwap_ys($svg);
    $want = expected_y($value, $min, $max, plot_geometry($svg));
    $svg_ok = count($ys) === count($rows) - 1
        && max(array_map('abs', array_map(fn($y) => $y - $want, $ys))) <= 1;

    $im = imagecreatefromstring($chart->renderPng());
    $blue = 0;
    for ($y = $want - 1; $y <= $want + 1; $y++) {
        for ($x = 0; $x < 600; $x++) {
            if ((imagecolorat($im, $x, $y) & 0xFFFFFF) === 0x0000FF) $blue++;
        }
    }
    echo "$name: svg=", ($svg_ok ? 'ok' : 'BAD'), " png=", ($blue > 100 ? 'ok' : 'BAD'), "\n";
}

$price = [];
$no_volume = [];
$huge_volume = [];
for ($i = 0; $i < 5; $i++) {
    $price[] = [1700000000 + $i * 86400, 1e308, 1e308, 1e308, 1e308, 100];
    $no_volume[] = [1700000000 + $i * 86400, 1e308, 1e308, 1e308, 1e308];
    $huge_volume[] = [1700000000 + $i * 86400, 100, 100, 100, 100, 1e308];
}
check_vwap('extreme_price', $price, 1e308, 9e307, 1e308);
check_vwap('no_volume_fallback', $no_volume, 1e308, 9e307, 1e308);
check_vwap('extreme_volume', $huge_volume, 100, 0, 200);

$residual_volume = [];
$residual_no_volume = [];
for ($i = 0; $i < 3; $i++) {
    $residual_volume[] = [1700000000 + $i * 86400, 1e-100, 1e308, -1e308, 1e-100, 100];
    $residual_no_volume[] = [1700000000 + $i * 86400, 1e-100, 1e308, -1e308, 1e-100];
}
check_vwap('residual_volume', $residual_volume, 1e-100 / 3, 0, 1e-100);
check_vwap('residual_no_volume', $residual_no_volume, 1e-100 / 3, 0, 1e-100);
?>
--EXPECT--
extreme_price: svg=ok png=ok
no_volume_fallback: svg=ok png=ok
extreme_volume: svg=ok png=ok
residual_volume: svg=ok png=ok
residual_no_volume: svg=ok png=ok
