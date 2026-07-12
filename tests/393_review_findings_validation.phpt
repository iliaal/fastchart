--TEST--
Review findings: numeric, cap, and scalar setter validation preserves state
--EXTENSIONS--
fastchart
--FILE--
<?php

function survives(string $label, object $chart, callable $invalid): void
{
    $before = $chart->renderSvg();
    try {
        $invalid($chart);
        echo "$label: NO THROW\n";
        return;
    } catch (ValueError $e) {
    }
    echo "$label: ", $chart->renderSvg() === $before ? "survives" : "CORRUPTED", "\n";
}

survives(
    'pie_total',
    (new FastChart\PieChart(300, 220))
        ->setSlices(['large' => 9, 'small' => 1])
        ->setOtherThreshold(0.2),
    fn ($c) => $c->setSlices(['a' => PHP_FLOAT_MAX, 'b' => PHP_FLOAT_MAX])
);

survives(
    'pie_ring_total',
    (new FastChart\PieChart(300, 220))->setRings([[['value' => 1], ['value' => 2]]]),
    fn ($c) => $c->setRings([[['value' => PHP_FLOAT_MAX], ['value' => PHP_FLOAT_MAX]]])
);

survives(
    'line_error_bars',
    (new FastChart\LineChart(300, 220))->setSeries([1, 2])->setErrorBars([1, 1]),
    fn ($c) => $c->setErrorBars(array_fill(0, 2049, 1))
);

survives(
    'scatter_error_bars',
    (new FastChart\ScatterChart(300, 220))->setPoints([[1, 2], [2, 3]])->setErrorBars([1, 1]),
    fn ($c) => $c->setErrorBars(array_fill(0, 4097, 1))
);

survives(
    'venn_sets',
    (new FastChart\VennDiagram(300, 220))->setSets([
        ['label' => 'a'], ['label' => 'b'], ['label' => 'c'],
    ]),
    fn ($c) => $c->setSets([
        ['label' => 'w'], ['label' => 'x'], ['label' => 'y'], ['label' => 'z'],
    ])
);

survives(
    'pictogram_total',
    (new FastChart\Pictogram(300, 160))->setTotal(10)->setValue(5),
    fn ($c) => $c->setTotal(-1)
);

survives(
    'scatter_href',
    (new FastChart\ScatterChart(300, 220))->setPoints([
        [1, 2, 'href' => 'https://example.com/ok'],
    ]),
    fn ($c) => $c->setPoints([
        [1, 2, 'href' => 'https://example.com/' . str_repeat('x', 4097)],
    ])
);

survives(
    'scatter_tooltip',
    (new FastChart\ScatterChart(300, 220))->setPoints([
        [1, 2, 'tooltip' => 'ok'],
    ]),
    fn ($c) => $c->setPoints([
        [1, 2, 'tooltip' => str_repeat('x', 4097)],
    ])
);

survives(
    'network_label_budget',
    (new FastChart\NetworkChart(300, 220))->setIterations(1)->setNodes([
        ['label' => 'a'], ['label' => 'b'],
    ]),
    fn ($c) => $c->setNodes(array_fill(0, 9, ['label' => str_repeat('W', 8192)]))
);

survives(
    'arc_label_budget',
    (new FastChart\ArcDiagram(300, 220))
        ->setNodes([['label' => 'a'], ['label' => 'b']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 1]]),
    fn ($c) => $c->setNodes(array_fill(0, 9, ['label' => str_repeat('W', 8192)]))
);

survives(
    'chord_label_budget',
    (new FastChart\ChordDiagram(300, 220))
        ->setNodes([['label' => 'a'], ['label' => 'b']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 1]]),
    fn ($c) => $c->setNodes(array_fill(0, 9, ['label' => str_repeat('W', 8192)]))
);

$long = str_repeat('L', 8193);
$with = (new FastChart\LineChart(300, 180))->setSeries([1])->setCategoryLabels([$long])->renderSvg();
$without = (new FastChart\LineChart(300, 180))->setSeries([1])->setCategoryLabels([])->renderSvg();
echo 'category_label_dropped: ', $with === $without ? "yes\n" : "NO\n";

function throws_value_error(string $label, callable $call): void
{
    try {
        $call();
        echo "$label: NO THROW\n";
    } catch (ValueError $e) {
        echo "$label: throws\n";
    }
}

throws_value_error('pictogram_shape', fn () => (new FastChart\Pictogram())->setShape(99));
throws_value_error('pictogram_fill', fn () => (new FastChart\Pictogram())->setFillColor(0x1000000));
throws_value_error('pictogram_empty', fn () => (new FastChart\Pictogram())->setEmptyColor(-2));

$stock = (new FastChart\StockChart())->setOhlcv([
    [1, 10, 12, 9, 11, 100], [2, 11, 13, 10, 12, 100],
]);
throws_value_error('vwap_color', fn () => $stock->addVWAP(0x1000000));

?>
--EXPECT--
pie_total: survives
pie_ring_total: survives
line_error_bars: survives
scatter_error_bars: survives
venn_sets: survives
pictogram_total: survives
scatter_href: survives
scatter_tooltip: survives
network_label_budget: survives
arc_label_budget: survives
chord_label_budget: survives
category_label_dropped: yes
pictogram_shape: throws
pictogram_fill: throws
pictogram_empty: throws
vwap_color: throws
