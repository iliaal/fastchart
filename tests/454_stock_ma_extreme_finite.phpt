--TEST--
StockChart SMA/EMA/WMA remain accurate for finite extreme closes
--EXTENSIONS--
fastchart
gd
--SKIPIF--
<?php
if (!function_exists('imagecreatefromstring')) die('skip gd required');
?>
--FILE--
<?php

function overlay_lines(string $svg): array {
    preg_match_all('/<line x1="([-0-9.]+)" y1="([-0-9.]+)" x2="([-0-9.]+)" y2="([-0-9.]+)" stroke="[^"]*" stroke-width="2"/', $svg, $m, PREG_SET_ORDER);
    $out = [];
    foreach ($m as $line) {
        $out[] = [(float)$line[1], (float)$line[2]];
        $out[] = [(float)$line[3], (float)$line[4]];
    }
    return $out;
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

$rows = [
    [1700000000, 1e308, 1e308, 1e308, 1e308, 100],
    [1700086400, 9e307, 9e307, 9e307, 9e307, 100],
    [1700172800, 8e307, 8e307, 8e307, 8e307, 100],
    [1700259200, 8e307, 8e307, 8e307, 8e307, 100],
];
$min = 8e307;
$max = 1e308;
$mk = fn() => (new FastChart\StockChart(600, 400))
    ->setOhlcv($rows)
    ->setYAxisRange($min, $max)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);

$cases = [
    'SMA' => [FastChart\StockChart::MA_SMA, 9e307],
    'EMA' => [FastChart\StockChart::MA_EMA, 9e307],
    'WMA' => [FastChart\StockChart::MA_WMA, 8e307 + (2e307 / 3)],
];
foreach ($cases as $name => [$type, $value]) {
    $chart = $mk()->addMovingAverage(3, $type);
    $svg = $chart->renderSvg();
    $ys = array_column(overlay_lines($svg), 1);
    $plot = plot_geometry($svg);
    $want = expected_y($value, $min, $max, $plot);
    $svg_ok = count($ys) === 2 && abs($ys[0] - $want) <= 1;

    $im = imagecreatefromstring($chart->renderPng());
    $png_hits = 0;
    $png_max_y = 0;
    for ($y = 0; $y < 400; $y++) {
        for ($x = 0; $x < 600; $x++) {
            if ((imagecolorat($im, $x, $y) & 0xFFFFFF) === 0x1F77B4) {
                $png_hits++;
                $png_max_y = max($png_max_y, $y);
            }
        }
    }
    $png_ok = $png_hits > 100 && $png_max_y < $plot[0] - 5;
    echo "$name: svg=", ($svg_ok ? 'ok' : 'BAD'), " png=", ($png_ok ? 'ok' : 'BAD'), "\n";
}

$residual_rows = [];
foreach ([1e308, -1e308, 1e-100, 1e-100, 1e-100, 1e-100] as $i => $close) {
    $residual_rows[] = [1700000000 + $i * 86400, $close, $close, $close, $close, 100];
}
foreach ($cases as $name => [$type]) {
    $chart = (new FastChart\StockChart(600, 400))
        ->setOhlcv($residual_rows)
        ->setYAxisRange(0, 1e-100)
        ->setLegendPosition(FastChart\Chart::LEGEND_NONE)
        ->addMovingAverage(3, $type);
    $svg = $chart->renderSvg();
    $ys = array_column(overlay_lines($svg), 1);
    $svg_ok = count($ys) >= 4 && $ys[count($ys) - 1] < 100;

    $im = imagecreatefromstring($chart->renderPng());
    $png_hits = 0;
    for ($y = 0; $y < 100; $y++) {
        for ($x = 0; $x < 600; $x++) {
            if ((imagecolorat($im, $x, $y) & 0xFFFFFF) === 0x1F77B4) $png_hits++;
        }
    }
    echo "$name residual: svg=", ($svg_ok ? 'ok' : 'BAD'),
        " png=", ($png_hits > 100 ? 'ok' : 'BAD'), "\n";
}
?>
--EXPECT--
SMA: svg=ok png=ok
EMA: svg=ok png=ok
WMA: svg=ok png=ok
SMA residual: svg=ok png=ok
EMA residual: svg=ok png=ok
WMA residual: svg=ok png=ok
