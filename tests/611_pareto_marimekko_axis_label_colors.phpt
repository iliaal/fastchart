--TEST--
ParetoChart and MarimekkoChart apply axis-label colors to tick and category labels only
--EXTENSIONS--
fastchart
--FILE--
<?php
function paretoChart(): FastChart\ParetoChart {
    return (new FastChart\ParetoChart(600, 400))
        ->setBars([
            ['label' => 'Alpha', 'value' => 50],
            ['label' => 'Beta', 'value' => 30],
            ['label' => 'Gamma', 'value' => 20],
        ])
        ->setShowValues(true)
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        // Native SVG does not open the font file; avoid host font dependence.
        ->setFontPath(__FILE__);
}
function marimekkoChart(): FastChart\MarimekkoChart {
    return (new FastChart\MarimekkoChart(600, 400))
        ->setColumns([
            ['label' => 'Q1', 'segments' => [['label' => 'Hardware', 'value' => 30]]],
            ['label' => 'Q2', 'segments' => [['label' => 'Services', 'value' => 20]]],
        ])
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setFontPath(__FILE__);
}
function labels(string $svg): array {
    preg_match_all('/<text\b[^>]*fill="(#[0-9a-f]{6})"(?:[^>]*text-anchor="([^"]+)")?[^>]*>([^<]*)<\/text>/i', $svg, $m, PREG_SET_ORDER);
    return array_map(static fn(array $row): array => [strtolower($row[1]), $row[2], $row[3]], $m);
}
function checkPareto(string $name, FastChart\ParetoChart $chart, string $axis, string $text): void {
    $expected = [];
    foreach (['0', '10', '20', '30', '40', '50'] as $v) $expected[] = [$axis, 'end', $v];
    foreach (['0%', '25%', '50%', '75%', '100%'] as $v) $expected[] = [$axis, '', $v];
    foreach (['Alpha' => '50', 'Beta' => '30', 'Gamma' => '20'] as $label => $value) {
        $expected[] = [$axis, 'middle', $label];
        $expected[] = [$text, 'middle', $value];
    }
    $actual = labels($chart->renderSvg());
    echo $name, ': ', $actual === $expected ? 'OK' : 'FAIL ' . json_encode($actual), "\n";
}
function checkMarimekko(string $name, FastChart\MarimekkoChart $chart, string $axis, string $text): void {
    $expected = [
        [$text, 'middle', 'Hardware'], [$axis, 'middle', 'Q1'],
        [$text, 'middle', 'Services'], [$axis, 'middle', 'Q2'],
    ];
    $actual = labels($chart->renderSvg());
    echo $name, ': ', $actual === $expected ? 'OK' : 'FAIL ' . json_encode($actual), "\n";
}

$theme = paretoChart()->renderSvg();
$chart = paretoChart()->setTextColor(0x135724);
$fallback = $chart->renderSvg();
checkPareto('pareto text fallback', $chart, '#135724', '#135724');
checkPareto('pareto override', $chart->setAxisLabelColor(0xAB12CD), '#ab12cd', '#135724');
checkPareto('pareto black override', $chart->setAxisLabelColor(0), '#000000', '#135724');
echo 'pareto reset to text: ', $chart->setAxisLabelColor(-1)->renderSvg() === $fallback ? 'OK' : 'FAIL', "\n";
echo 'pareto reset to theme: ', $chart->setTextColor(-1)->renderSvg() === $theme ? 'OK' : 'FAIL', "\n";
checkPareto('pareto default', paretoChart(), '#222222', '#222222');

$theme = marimekkoChart()->renderSvg();
$chart = marimekkoChart()->setTextColor(0x135724);
$fallback = $chart->renderSvg();
checkMarimekko('marimekko text fallback', $chart, '#135724', '#135724');
checkMarimekko('marimekko override', $chart->setAxisLabelColor(0xAB12CD), '#ab12cd', '#135724');
checkMarimekko('marimekko black override', $chart->setAxisLabelColor(0), '#000000', '#135724');
echo 'marimekko reset to text: ', $chart->setAxisLabelColor(-1)->renderSvg() === $fallback ? 'OK' : 'FAIL', "\n";
echo 'marimekko reset to theme: ', $chart->setTextColor(-1)->renderSvg() === $theme ? 'OK' : 'FAIL', "\n";
checkMarimekko('marimekko default', marimekkoChart(), '#222222', '#222222');

$withoutTextColor = static fn(string $svg): string => preg_replace('/(<text\b[^>]*fill=")#[0-9a-f]{6}/i', '${1}COLOR', $svg);
echo 'pareto geometry unchanged: ', $withoutTextColor(paretoChart()->renderSvg())
    === $withoutTextColor(paretoChart()->setAxisLabelColor(0xAB12CD)->renderSvg()) ? 'OK' : 'FAIL', "\n";
echo 'marimekko geometry unchanged: ', $withoutTextColor(marimekkoChart()->renderSvg())
    === $withoutTextColor(marimekkoChart()->setAxisLabelColor(0xAB12CD)->renderSvg()) ? 'OK' : 'FAIL', "\n";
?>
--EXPECT--
pareto text fallback: OK
pareto override: OK
pareto black override: OK
pareto reset to text: OK
pareto reset to theme: OK
pareto default: OK
marimekko text fallback: OK
marimekko override: OK
marimekko black override: OK
marimekko reset to text: OK
marimekko reset to theme: OK
marimekko default: OK
pareto geometry unchanged: OK
marimekko geometry unchanged: OK
