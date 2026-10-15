--TEST--
Line, Scatter, and Cartesian Bar preserve complete shown value labels
--EXTENSIONS--
fastchart
--FILE--
<?php
use FastChart\Chart;
use FastChart\LineChart;
use FastChart\BarChart;
use FastChart\ScatterChart;

$cases = [
    'line gaps' => [(new LineChart(600, 400))->setSeries([50, NAN, -25, 0]), [50, -25, 0]],
    'line secondary' => [(new LineChart(600, 400))->setSeries([
        ['data' => [50, 25]], ['data' => [100, 75], 'axis' => 'right'],
    ])->setSecondaryYAxis(true), [50, 25, 100, 75]],
    'scatter' => [(new ScatterChart(600, 400))->setPoints([[1, 50], [2, -25], [3, 0]]), [50, -25, 0]],
];
foreach ([BarChart::BAR_VERTICAL, BarChart::BAR_HORIZONTAL] as $orientation) {
    $bar = (new BarChart(600, 400))->setOrientation($orientation)->setSeries([50, NAN, -25, 0]);
    $cases["bar $orientation"] = [$bar, [50, -25, 0]];
    $cases["lollipop $orientation"] = [(clone $bar)->setBarStyle(BarChart::BAR_STYLE_LOLLIPOP), [50, -25, 0]];
    $cases["single stacked $orientation"] = [(clone $bar)->setStacked(true), [50, -25, 0]];
    $grouped = (new BarChart(600, 400))->setOrientation($orientation)->setSeries([
        ['data' => [50, -25]], ['data' => [25, 0]],
    ]);
    $cases["grouped $orientation"] = [$grouped, [50, 25, -25, 0]];
    $cases["beside $orientation"] = [(clone $grouped)->setStacked(true)->setStackMode(Chart::STACK_BESIDE), [50, 25, -25, 0]];
    $cases["stacked hidden $orientation"] = [(clone $grouped)->setStacked(true), []];
    $cases["layered hidden $orientation"] = [(clone $grouped)->setStackMode(Chart::STACK_LAYER), []];
    $floating = (new BarChart(600, 400))->setOrientation($orientation)
        ->setFloating(true)->setSeries([[10, 50], [25, 60]]);
    $cases["floating $orientation"] = [$floating, [10, 25]];
    $cases["dumbbell $orientation"] = [(clone $floating)->setBarStyle(BarChart::BAR_STYLE_DUMBBELL), [10, 25]];
}
$cases['radial hidden'] = [(new BarChart(600, 400))->setOrientation(BarChart::BAR_RADIAL)->setSeries([50, 25]), []];

foreach ($cases as $name => [$chart, $values]) {
    $chart->setSvgTextMode(Chart::SVG_TEXT_NATIVE);
    $hidden = $chart->setShowValues(false)->renderSvg();
    $default = $chart->setShowValues(true, '')->renderSvg();
    $ok = true;
    // Including the literal prefix, these widths straddle the old 31-byte limit.
    foreach (['v%30.0f', 'v%31.0f', 'v%32.0f', 'v%100.2f', 'v%.300f', 'v<%.2f>&%%'] as $format) {
        $chart->setShowValues(true, $format);
        $svg = $chart->renderSvg();
        preg_match_all('/<text\b[^>]*>(v[^<]*)<\/text>/', $svg, $matches);
        $expected = [];
        foreach ($values as $value) {
            // PHP sprintf caps precision at 53; the accepted C format does not.
            $text = $format === 'v%.300f' ? 'v' . $value . '.' . str_repeat('0', 300)
                : sprintf($format, $value);
            $expected[] = htmlspecialchars($text, ENT_QUOTES | ENT_XML1, 'UTF-8');
        }
        if ($matches[1] !== $expected) {
            echo "$name $format: incomplete labels\n";
            $ok = false;
        }
        $ok = $ok && $svg === $chart->renderSvg();
        $ok = $ok && $hidden === $chart->setShowValues(false)->renderSvg();
        $ok = $ok && $svg === $chart->setShowValues(true, null)->renderSvg();
    }
    $ok = $ok && $default === $chart->setShowValues(true, '')->renderSvg();
    echo "$name: ", $ok ? 'complete and stable' : 'FAIL', "\n";
}
?>
--EXPECT--
line gaps: complete and stable
line secondary: complete and stable
scatter: complete and stable
bar 0: complete and stable
lollipop 0: complete and stable
single stacked 0: complete and stable
grouped 0: complete and stable
beside 0: complete and stable
stacked hidden 0: complete and stable
layered hidden 0: complete and stable
floating 0: complete and stable
dumbbell 0: complete and stable
bar 1: complete and stable
lollipop 1: complete and stable
single stacked 1: complete and stable
grouped 1: complete and stable
beside 1: complete and stable
stacked hidden 1: complete and stable
layered hidden 1: complete and stable
floating 1: complete and stable
dumbbell 1: complete and stable
radial hidden: complete and stable
