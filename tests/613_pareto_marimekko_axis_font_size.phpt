--TEST--
Pareto and Marimekko separate axis and data-label font sizes
--EXTENSIONS--
fastchart
--FILE--
<?php
function chart(string $kind): FastChart\Chart {
    $chart = $kind === 'pareto'
        ? (new FastChart\ParetoChart(600, 400))->setBars([
            ['label' => 'Alpha', 'value' => 50],
            ['label' => 'Beta', 'value' => 30],
            ['label' => 'Gamma', 'value' => 20],
        ])->setShowValues(true)
        : (new FastChart\MarimekkoChart(600, 400))->setColumns([
            ['label' => 'Q1', 'segments' => [['label' => 'Hardware', 'value' => 30]]],
            ['label' => 'Q2', 'segments' => [['label' => 'Services', 'value' => 20]]],
        ]);
    return $chart->setFontPath(__FILE__)->setFontSize(12)
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setAxisLabelColor(0xAB12CD);
}
function check(string $name, string $kind, FastChart\Chart $chart, float $axis, float $label): void {
    $svg = $chart->renderSvg();
    preg_match_all('/<text\b[^>]*font-size="([^"]+)"[^>]*fill="([^"]+)"[^>]*>([^<]*)<\/text>/', $svg, $matches, PREG_SET_ORDER);
    $axes = $values = [];
    foreach ($matches as $row) {
        $isAxis = strtolower($row[2]) === '#ab12cd';
        $expected = ($isAxis ? $axis : $label * ($kind === 'pareto' ? 0.9 : 1)) * 4 / 3;
        if (abs((float)$row[1] - $expected) > 0.051) {
            echo "$kind $name: FAIL size {$row[1]} for {$row[3]}\n";
            return;
        }
        if ($isAxis) $axes[] = $row[3]; else $values[] = $row[3];
    }
    $expectedAxes = $kind === 'pareto'
        ? ['0', '10', '20', '30', '40', '50', '0%', '25%', '50%', '75%', '100%', 'Alpha', 'Beta', 'Gamma']
        : ['Q1', 'Q2'];
    $expectedValues = $kind === 'pareto' ? ['50', '30', '20'] : ['Hardware', 'Services'];
    echo "$kind $name: ", $axes === $expectedAxes && $values === $expectedValues && $chart->renderSvg() === $svg ? 'OK' : 'FAIL labels/repeat', "\n";
}
foreach (['pareto', 'marimekko'] as $kind) {
    check('global fallback', $kind, chart($kind), 12, 12);
    check('axis only', $kind, chart($kind)->setAxisFont(null, 21), 21, 12);
    check('label only', $kind, chart($kind)->setLabelFont(null, 15), 12, 15);
    $chart = chart($kind)->setAxisFont(null, 21)->setLabelFont(null, 15);
    check('both roles', $kind, $chart, 21, 15);
    // Axis changes must not alter data-label position, fit, or non-text geometry.
    $withoutAxes = static fn(string $svg): string => preg_replace('/<text\b[^>]*fill="#AB12CD"[^>]*>[^<]*<\/text>\n?/i', '', $svg);
    $before = $withoutAxes($chart->renderSvg());
    $chart->setAxisFont(null, 120);
    echo "$kind axis isolated: ", $withoutAxes($chart->renderSvg()) === $before ? 'OK' : 'FAIL', "\n";
    check('updated roles', $kind, $chart->setAxisFont(null, 18)->setLabelFont(null, 9), 18, 9);
    check('thumbnail scaling', $kind, $chart->setThumbnailMode(true), 10.8, 5.4);
}
?>
--EXPECT--
pareto global fallback: OK
pareto axis only: OK
pareto label only: OK
pareto both roles: OK
pareto axis isolated: OK
pareto updated roles: OK
pareto thumbnail scaling: OK
marimekko global fallback: OK
marimekko axis only: OK
marimekko label only: OK
marimekko both roles: OK
marimekko axis isolated: OK
marimekko updated roles: OK
marimekko thumbnail scaling: OK
