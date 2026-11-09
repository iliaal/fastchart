--TEST--
Pareto and Marimekko apply and clear axis font paths independently of data-label fonts
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die("skip no system font available\n");
?>
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';
function chart(string $kind, string $font): FastChart\Chart {
    $chart = $kind === 'pareto'
        ? (new FastChart\ParetoChart(600, 400))->setBars([['label' => 'Category', 'value' => 50]])->setShowValues(true)
        : (new FastChart\MarimekkoChart(600, 400))->setColumns([['label' => 'Category', 'segments' => [['label' => 'Segment', 'value' => 50]]]]);
    return $chart->setFontPath($font)->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)->setAxisLabelColor(0xAB12CD);
}
function families(FastChart\Chart $chart): array {
    preg_match_all('/<text\b[^>]*font-family="([^"]+)"[^>]*fill="([^"]+)"[^>]*>[^<]*<\/text>/', $chart->renderSvg(), $matches, PREG_SET_ORDER);
    $roles = ['axis' => [], 'label' => []];
    foreach ($matches as $row) $roles[strtolower($row[2]) === '#ab12cd' ? 'axis' : 'label'][] = $row[1];
    return $roles;
}
foreach (['pareto', 'marimekko'] as $kind) {
    // A real font and an existing non-font file give distinct native families
    // without depending on the name of the installed font.
    $base = families(chart($kind, __FILE__));
    $real = families(chart($kind, fc_pick_font()));
    echo "$kind distinct fixtures: ", $base['axis'] !== $real['axis'] && $base['label'] !== $real['label'] ? 'OK' : 'FAIL', "\n";
    $chart = chart($kind, __FILE__)->setAxisFont(fc_pick_font());
    echo "$kind axis path: ", families($chart) === ['axis' => $real['axis'], 'label' => $base['label']] ? 'OK' : 'FAIL', "\n";
    echo "$kind clear axis path: ", families($chart->setAxisFont('')) === $base ? 'OK' : 'FAIL', "\n";
    $chart->setLabelFont(fc_pick_font());
    echo "$kind label path: ", families($chart) === ['axis' => $base['axis'], 'label' => $real['label']] ? 'OK' : 'FAIL', "\n";
    echo "$kind both paths: ", families($chart->setAxisFont(fc_pick_font())) === $real ? 'OK' : 'FAIL', "\n";
    echo "$kind clear label path: ", families($chart->setLabelFont('')) === ['axis' => $real['axis'], 'label' => $base['label']] ? 'OK' : 'FAIL', "\n";
}
?>
--EXPECT--
pareto distinct fixtures: OK
pareto axis path: OK
pareto clear axis path: OK
pareto label path: OK
pareto both paths: OK
pareto clear label path: OK
marimekko distinct fixtures: OK
marimekko axis path: OK
marimekko clear axis path: OK
marimekko label path: OK
marimekko both paths: OK
marimekko clear label path: OK
