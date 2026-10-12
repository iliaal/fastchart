--TEST--
ParetoChart preserves complete formatted axis ticks and bar values
--EXTENSIONS--
fastchart
--FILE--
<?php
$chart = (new FastChart\ParetoChart(600, 400))
    ->setBars([['value' => 50], ['value' => 25]])
    ->setYAxisRange(0, 50)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
function labels(string $svg): array {
    preg_match_all('/<text\b[^>]*>([^<]*)<\/text>/', $svg, $matches);
    return $matches[1];
}
foreach (['%31.0f', '%32.0f', '%33.0f', '%100.2f', '%.300f', 'value: <%.2f>&%%'] as $format) {
    $chart->setShowValues(true)->setValueFormat($format);
    $svg = $chart->renderSvg();
    $expected = [];
    foreach ([0, 10, 20, 30, 40, 50, 50, 25] as $value) {
        // PHP sprintf caps precision at 53, but the accepted C format does not.
        $label = $format === '%.300f' ? $value . '.' . str_repeat('0', 300)
            : sprintf($format, $value);
        $expected[] = htmlspecialchars($label, ENT_QUOTES | ENT_XML1, 'UTF-8');
    }
    $actual = labels($svg);
    echo array_slice($actual, 0, 6) === array_slice($expected, 0, 6) ? "ticks complete\n" : "TICKS TRUNCATED\n";
    echo array_slice($actual, 11) === array_slice($expected, 6) ? "bars complete\n" : "BARS TRUNCATED\n";
    echo array_slice($actual, 6, 5) === ['0%', '25%', '50%', '75%', '100%'] ? "percentages unchanged\n" : "BAD PERCENTAGES\n";
    echo $svg === $chart->renderSvg() ? "stable\n" : "UNSTABLE\n";
    $chart->setShowValues(false);
    echo labels($chart->renderSvg()) === array_slice($actual, 0, 11) ? "bars hidden\n" : "BAD HIDDEN VALUES\n";
}
$chart->setShowValues(true)->setValueFormat('');
echo labels($chart->renderSvg()) === ['0', '10', '20', '30', '40', '50',
    '0%', '25%', '50%', '75%', '100%', '50', '25'] ? "default\n" : "BAD DEFAULT\n";
?>
--EXPECT--
ticks complete
bars complete
percentages unchanged
stable
bars hidden
ticks complete
bars complete
percentages unchanged
stable
bars hidden
ticks complete
bars complete
percentages unchanged
stable
bars hidden
ticks complete
bars complete
percentages unchanged
stable
bars hidden
ticks complete
bars complete
percentages unchanged
stable
bars hidden
ticks complete
bars complete
percentages unchanged
stable
bars hidden
default
