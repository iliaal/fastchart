--TEST--
Custom-format axis tick labels use their own 64 KiB per-render budget
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
$c = (new FastChart\LineChart(600,400))->setSeries([1,2])->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
if (!str_contains($c->renderSvg(), '<text ')) echo 'skip no usable default font';
?>
--FILE--
<?php
use FastChart\Chart;

$pad = str_repeat('x', 8000);
$fmt = $pad . '%.0f';

function count_labels(Chart $c, string $prefix): int {
    $svg = $c->setSvgTextMode(Chart::SVG_TEXT_NATIVE)->renderSvg();
    return substr_count($svg, '>' . $prefix);
}

$rows = [];
for ($i = 0; $i < 100; $i++) {
    $rows[] = [1700000000 + $i * 86400, 10, 12, 9, 11, 100];
}
$stock = fn() => (new FastChart\StockChart(1600, 600))
    ->setOhlcv($rows)
    ->setDateAxisStride(Chart::DATE_DAY, 1);

/* Each 1.7e9 timestamp formats to 8000 + 10 bytes. */
echo "date ticks: ", count_labels($stock()->setXAxisLabelFormat($fmt), $pad),
    " of ", intdiv(65536, 8010), "\n";

/* Left and right value axes draw from the same tick channel. */
$line = fn() => (new FastChart\LineChart(800, 1200))
    ->setSeries([['data' => range(0, 15)],
                 ['data' => range(0, 15), 'axis' => 'right']])
    ->setSecondaryYAxis(true);
echo "y ticks, both axes: ", count_labels($line()->setYAxisLabelFormat($pad . '%.1f'), $pad),
    " of ", intdiv(65536, 8003), "\n";

$both = $line()->setYAxisLabelFormat($pad . '%.1f')
    ->setShowValues(true, 'v=%.0f');
$svg = $both->setSvgTextMode(Chart::SVG_TEXT_NATIVE)->renderSvg();
echo "value labels beside exhausted ticks: ", substr_count($svg, '>v='), "\n";

$pareto = (new FastChart\ParetoChart(800, 600))
    ->setBars([['label' => 'a', 'value' => 5]])->setValueFormat($pad . '%.0f');
echo "pareto ticks: ", count_labels($pareto, $pad), "\n";

try {
    $stock()->setXAxisLabelFormat($fmt)->setStrict(true)->renderSvg();
    echo "strict: no exception\n";
} catch (ValueError $e) {
    echo "strict: ", $e->getMessage(), "\n";
}
?>
--EXPECT--
date ticks: 8 of 8
y ticks, both axes: 8 of 8
value labels beside exhausted ticks: 32
pareto ticks: 6
strict: FastChart: axis tick labels exceed the 65536-byte per-render text limit
