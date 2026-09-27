--TEST--
StockChart VWAP carries the running value through candles without usable volume
--EXTENSIONS--
fastchart
--FILE--
<?php

function blue_ys(string $svg): array {
    preg_match_all('/<line x1="[-0-9.]+" y1="([-0-9.]+)" x2="[-0-9.]+" y2="([-0-9.]+)" stroke="#0000FF" stroke-width="2"/', $svg, $m, PREG_SET_ORDER);
    if (!$m) return [];
    $out = [(float)$m[0][1]];
    foreach ($m as $line) $out[] = (float)$line[2];
    return $out;
}

function plot_geometry(string $svg): array {
    preg_match_all('/<rect x="[0-9.-]+" y="([0-9.-]+)" width="[0-9.-]+" height="([0-9.-]+)" fill="#FFFFFF"/', $svg, $m);
    $height = (float)$m[2][1] - 1.0;
    return [(float)$m[1][1] + $height, $height];
}

function expected_y(float $value, float $min, float $max, array $plot): int {
    [$bottom, $height] = $plot;
    return (int)($bottom - (int)((($value - $min) / ($max - $min)) * $height + 0.5));
}

$rows = [
    [1700000000, 100, 100, 100, 100, 1000],
    [1700086400, 200, 200, 200, 200, null],
    [1700172800, 300, 300, 300, 300],
    [1700259200, 400, 400, 400, 400, 1000],
];
$svg = (new FastChart\StockChart(600, 400))
    ->setOhlcv($rows)
    ->setYAxisRange(0, 500)
    ->setLegendPosition(FastChart\Chart::LEGEND_NONE)
    ->addVWAP(0x0000FF)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->renderSvg();

$plot = plot_geometry($svg);
$expected = array_map(
    fn (float $value) => expected_y($value, 0, 500, $plot),
    [100.0, 100.0, 100.0, 250.0]
);
$actual = blue_ys($svg);
echo 'carry_forward: ', ($actual === array_map('floatval', $expected) ? 'ok' : 'BAD'), "\n";
?>
--EXPECT--
carry_forward: ok
