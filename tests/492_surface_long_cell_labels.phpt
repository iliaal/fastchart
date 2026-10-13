--TEST--
SurfaceChart preserves complete formatted cell labels
--EXTENSIONS--
fastchart
--FILE--
<?php
use FastChart\Chart;
use FastChart\SurfaceChart;

function labels(string $svg): array {
    preg_match_all('/<text\b[^>]*>([^<]*)<\/text>/', $svg, $matches);
    return $matches[1];
}
$cases = [
    'single' => [(new SurfaceChart(600, 400))->setGrid([[50]]), [50]],
    'rectangular gaps' => [(new SurfaceChart(600, 400))->setGrid([[50, NAN, -25], [0, 25, NAN]]), [50, -25, 0, 25]],
    // Only the last column/row has a nonempty rectangle in a 1px plot.
    'dense' => [(new SurfaceChart(600, 400))->setGrid([[50, 25], [-25, 0]])
        ->setPlotRect(50, 20, 50, 20), [0]],
];
foreach ($cases as $name => [$chart, $values]) {
    $chart->setSvgTextMode(Chart::SVG_TEXT_NATIVE);
    $hidden = $chart->setShowCellValues(false)->renderSvg();
    $default = $chart->setShowCellValues(true)->renderSvg();
    $ok = true;
    foreach (['v%30.0f', 'v%31.0f', 'v%32.0f', 'v%100.2f', 'v%.300f', 'v<%.2f>&%%'] as $format) {
        $svg = $chart->setShowCellValues(true, $format)->renderSvg();
        $expected = [];
        foreach ($values as $value) {
            // PHP sprintf caps precision at 53; accepted C formats do not.
            $text = $format === 'v%.300f' ? 'v' . $value . '.' . str_repeat('0', 300)
                : sprintf($format, $value);
            $expected[] = htmlspecialchars($text, ENT_QUOTES | ENT_XML1, 'UTF-8');
        }
        if (labels($svg) !== $expected) {
            echo "$name $format: incomplete labels\n";
            $ok = false;
        }
        $ok = $ok && $svg === $chart->renderSvg();
        $ok = $ok && $svg === (clone $chart)->renderSvg();
        $ok = $ok && $hidden === $chart->setShowCellValues(false, $format)->renderSvg();
        $ok = $ok && $default === $chart->setShowCellValues(true)->renderSvg();
        $ok = $ok && $default === $chart->setShowCellValues(true, '')->renderSvg();
    }
    echo "$name: ", $ok ? 'complete and stable' : 'FAIL', "\n";
}
// The inherited generic label option is separate from Surface's cell option.
$chart = (new SurfaceChart(600, 400))->setGrid([[50]])->setSvgTextMode(Chart::SVG_TEXT_NATIVE);
echo labels($chart->setShowValues(true, 'v%.100f')->renderSvg()) === []
    ? "generic values unchanged\n" : "FAIL generic values\n";
?>
--EXPECT--
single: complete and stable
rectangular gaps: complete and stable
dense: complete and stable
generic values unchanged
