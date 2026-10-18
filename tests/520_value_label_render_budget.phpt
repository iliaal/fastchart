--TEST--
Per-element labels share one 64 KiB per-render text budget across chart families
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
$per = 8000 + 1;
$expected = intdiv(65536, $per);

function value_labels(Chart $c, string $pad): int {
    $svg = $c->setSvgTextMode(Chart::SVG_TEXT_NATIVE)->renderSvg();
    return substr_count($svg, '>' . $pad);
}

$charts = [
    'line' => (new FastChart\LineChart(800, 400))
        ->setSeries(array_fill(0, 16, 5))->setShowValues(true, $fmt),
    'bar' => (new FastChart\BarChart(800, 400))
        ->setSeries(array_fill(0, 16, 5))->setShowValues(true, $fmt),
    'scatter' => (new FastChart\ScatterChart(800, 400))
        ->setPoints(array_map(fn($i) => [$i, 5], range(0, 15)))
        ->setShowValues(true, $fmt),
    'funnel' => (new FastChart\Funnel(800, 400))
        ->setStages(array_fill(0, 16, ['label' => 's', 'value' => 5]))
        ->setShowValues(true, $fmt),
    'pareto' => (new FastChart\ParetoChart(800, 400))
        ->setBars(array_fill(0, 16, ['label' => 'b', 'value' => 5]))
        ->setValueFormat($fmt)->setShowValues(true),
    'surface' => (new FastChart\SurfaceChart(800, 400))
        ->setGrid(array_fill(0, 4, array_fill(0, 4, 5.0)))
        ->setShowCellValues(true, $fmt),
    'pie' => (new FastChart\PieChart(800, 400))
        ->setSlices(array_fill(0, 16, ['value' => 1]))
        ->setSliceLabelFormat($fmt),
    'radial bar categories' => (new FastChart\BarChart(800, 400))
        ->setOrientation(FastChart\BarChart::BAR_RADIAL)
        ->setSeries(array_fill(0, 16, 5))
        ->setCategoryLabels(array_fill(0, 16, $pad . '5')),
    'radar spokes' => (new FastChart\RadarChart(800, 400))
        ->setSeries([['data' => array_fill(0, 16, 5)]])
        ->setCategoryLabels(array_fill(0, 16, $pad . '5')),
];

foreach ($charts as $name => $chart) {
    /* Pareto's six left-axis ticks reuse the format; ticks are not
     * value labels. */
    $ticks = $name === 'pareto' ? 6 : 0;
    $first = value_labels($chart, $pad) - $ticks;
    $second = value_labels($chart, $pad) - $ticks;
    echo "$name: ", $first === $expected ? "capped at $expected" : "got $first",
        $second === $first ? ", rerender same" : ", rerender got $second", "\n";
}
?>
--EXPECT--
line: capped at 8, rerender same
bar: capped at 8, rerender same
scatter: capped at 8, rerender same
funnel: capped at 8, rerender same
pareto: capped at 8, rerender same
surface: capped at 8, rerender same
pie: capped at 8, rerender same
radial bar categories: capped at 8, rerender same
radar spokes: capped at 8, rerender same
