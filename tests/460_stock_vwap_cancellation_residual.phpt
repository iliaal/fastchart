--TEST--
StockChart VWAP preserves a tiny term after large opposite-sign cancellation
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

function check_residual(string $name, array $rows): void {
    $svg = (new FastChart\StockChart(600, 400))
        ->setOhlcv($rows)
        ->setYAxisRange(0, 1e-300)
        ->setLegendPosition(FastChart\Chart::LEGEND_NONE)
        ->addVWAP(0x0000FF)
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->renderSvg();
    [$bottom, $height] = plot_geometry($svg);
    $bottom = (int)$bottom;
    $third = (int)($bottom - (int)($height / 3.0 + 0.5));
    $top = (int)($bottom - $height);
    $ys = blue_ys($svg);
    $ok = count($ys) === 3
        && $ys[0] === (float)$top
        && $ys[1] === (float)$bottom
        && $ys[2] === (float)$third;
    echo "$name: ", ($ok ? 'ok' : 'BAD'), "\n";
}

$volume = [
    [1700000000, 1e300, 1e300, 1e300, 1e300, 1000],
    [1700086400, -1e300, -1e300, -1e300, -1e300, 1000],
    [1700172800, 1e-300, 1e-300, 1e-300, 1e-300, 1000],
];
$no_volume = array_map(static fn (array $row): array => array_slice($row, 0, 5), $volume);

check_residual('volume', $volume);
check_residual('no_volume', $no_volume);
?>
--EXPECT--
volume: ok
no_volume: ok
