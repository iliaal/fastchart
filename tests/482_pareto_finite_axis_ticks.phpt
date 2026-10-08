--TEST--
ParetoChart keeps left-axis tick labels finite for large finite ranges
--EXTENSIONS--
fastchart
--FILE--
<?php
function ticks(float $value, ?float $maximum = null): string {
    $chart = (new FastChart\ParetoChart(500, 300))
        ->setBars([['label' => 'A', 'value' => $value]])
        ->setValueFormat('%.1e')
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
    if ($maximum !== null) {
        $chart->setYAxisRange(0, $maximum);
    }
    preg_match_all('/<text\b[^>]*>([^<]*)<\/text>/', $chart->renderSvg(), $matches);
    // The six left-axis labels precede percentage ticks and bar labels.
    return implode(', ', array_slice($matches[1], 0, 6));
}

// Multiplying the maximum by the tick index before dividing by five
// overflows despite every intended tick lying in the finite axis range.
echo 'auto: ', ticks(1e308), "\n";
echo 'forced: ', ticks(50, 1e308), "\n";
echo 'max: ', ticks(50, PHP_FLOAT_MAX), "\n";
echo 'ordinary: ', ticks(50), "\n";
?>
--EXPECT--
auto: 0.0e+0, 2.0e+307, 4.0e+307, 6.0e+307, 8.0e+307, 1.0e+308
forced: 0.0e+0, 2.0e+307, 4.0e+307, 6.0e+307, 8.0e+307, 1.0e+308
max: 0.0e+0, 3.6e+307, 7.2e+307, 1.1e+308, 1.4e+308, 1.8e+308
ordinary: 0.0e+0, 1.0e+1, 2.0e+1, 3.0e+1, 4.0e+1, 5.0e+1
