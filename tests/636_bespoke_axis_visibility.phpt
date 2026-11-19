--TEST--
Pareto, Marimekko and Violin hide only the requested custom axis decorations
--EXTENSIONS--
fastchart
--FILE--
<?php
function chart(string $kind): FastChart\Chart {
    if ($kind === 'pareto') {
        $chart = (new FastChart\ParetoChart(600, 400))->setBars([
            ['label' => 'Alpha', 'value' => 50],
            ['label' => 'Beta', 'value' => 30],
            ['label' => 'Gamma', 'value' => 20],
        ])->setShowValues(true);
    } elseif ($kind === 'marimekko') {
        $chart = (new FastChart\MarimekkoChart(600, 400))->setColumns([
            ['label' => 'Alpha', 'segments' => [['label' => 'Hardware', 'value' => 30]]],
            ['label' => 'Beta', 'segments' => [['label' => 'Services', 'value' => 20]]],
        ]);
    } else {
        $chart = (new FastChart\ViolinPlot(600, 400))->setGroups([
            ['label' => 'Alpha', 'values' => [1, 2, 3, 5]],
            ['label' => 'Beta', 'values' => [2, 3, 4, 6]],
        ]);
    }
    return $chart->setFontPath(__FILE__)->setFontSize(12)
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setAxisLabelColor(0xAB12CD)->setGridColor(0x12AB34);
}
function axes(string $svg): array {
    preg_match_all('/<text\b[^>]*fill="#AB12CD"[^>]*>([^<]*)<\/text>/i', $svg, $matches);
    return $matches[1];
}
function grid(string $svg): array {
    preg_match_all('/<[^>]+\bstroke="#12AB34"[^>]*\/>/i', $svg, $matches);
    return $matches[0];
}
function series(string $svg): string {
    $svg = preg_replace('/<text\b[^>]*fill="#AB12CD"[^>]*>[^<]*<\/text>\n?/i', '', $svg);
    return preg_replace('/<[^>]+\bstroke="#12AB34"[^>]*\/>\n?/i', '', $svg);
}
function check(bool $ok, string $label): void {
    if (!$ok) throw new RuntimeException($label);
}
foreach (['pareto', 'marimekko', 'violin'] as $kind) {
    $chart = chart($kind);
    $original = $chart->renderSvg();
    $xLabels = $kind === 'pareto' ? ['Alpha', 'Beta', 'Gamma'] : ['Alpha', 'Beta'];
    $yLabels = $kind === 'pareto'
        ? ['0', '10', '20', '30', '40', '50', '0%', '25%', '50%', '75%', '100%'] : [];
    check(axes($original) === array_merge($yLabels, $xLabels), "$kind default axes");
    check(count(grid($original)) === ($kind === 'pareto' ? 6 : 0), "$kind default grid");
    foreach ($kind === 'pareto' ? ['50', '30', '20'] : ($kind === 'marimekko' ? ['Hardware', 'Services'] : []) as $label) {
        check(str_contains(series($original), ">$label</text>"), "$kind series label $label");
    }
    check($chart->setXAxisVisible(true)->setYAxisVisible(true)->renderSvg() === $original,
        "$kind explicit defaults");
    foreach ([[false, true], [true, false], [false, false]] as [$x, $y]) {
        $svg = $chart->setXAxisVisible($x)->setYAxisVisible($y)->renderSvg();
        check(axes($svg) === array_merge($y ? $yLabels : [], $x ? $xLabels : []), "$kind independent axes");
        check(grid($svg) === ($y ? grid($original) : []), "$kind Y grid visibility");
        check(series($svg) === series($original), "$kind series and geometry retained");
        check($chart->renderSvg() === $svg, "$kind repeat rendering");
        check($chart->setXAxisVisible(true)->setYAxisVisible(true)->renderSvg() === $original,
            "$kind re-enabled round trip");
    }
    echo "$kind: OK\n";
}
?>
--EXPECT--
pareto: OK
marimekko: OK
violin: OK
