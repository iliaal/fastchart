--TEST--
Every concrete FastChart class satisfies the object and clone contracts
--EXTENSIONS--
fastchart
--FILE--
<?php

function valid_svg(string $svg): bool
{
    return strlen($svg) > 100
        && str_contains($svg, '<svg')
        && str_contains($svg, '</svg>');
}

function build_object(Closure $builder): object
{
    $object = $builder();
    $object->setSvgTextMode(0);
    return $object;
}

function mutate_base(object $object, int $color): void
{
    if ($object instanceof FastChart\Chart) {
        $object->setBackgroundColor($color);
        return;
    }

    if ($object instanceof FastChart\Symbol) {
        $object->setBackground($color);
        return;
    }

    throw new LogicException('No base-field mutation for ' . $object::class);
}

function dynamic_property_blocked(object $object): bool
{
    try {
        $object->__fastchart_contract_probe = true;
        return false;
    } catch (Error) {
        return !property_exists($object, '__fastchart_contract_probe');
    }
}

function serialization_blocked(object $object): bool
{
    try {
        serialize($object);
        return false;
    } catch (Throwable) {
        return true;
    }
}

$ohlcv = [];
for ($i = 0; $i < 4; $i++) {
    $ohlcv[] = [1700000000 + $i * 86400,
        100 + $i, 102 + $i, 99 + $i, 101 + $i, 1000];
}

$builders = [
    FastChart\LineChart::class => fn() => (new FastChart\LineChart(240, 180))
        ->setSeries([1, 3, 2]),
    FastChart\AreaChart::class => fn() => (new FastChart\AreaChart(240, 180))
        ->setSeries([1, 3, 2]),
    FastChart\BarChart::class => fn() => (new FastChart\BarChart(240, 180))
        ->setSeries([1, 3, 2]),
    FastChart\PieChart::class => fn() => (new FastChart\PieChart(240, 180))
        ->setSlices(['a' => 1, 'b' => 2]),
    FastChart\ScatterChart::class => fn() => (new FastChart\ScatterChart(240, 180))
        ->setPoints([[1, 1], [2, 3], [3, 2]]),
    FastChart\StockChart::class => fn() => (new FastChart\StockChart(300, 200))
        ->setOhlcv($ohlcv),
    FastChart\RadarChart::class => fn() => (new FastChart\RadarChart(220, 220))
        ->setSeries([['data' => [3, 4, 2]]])
        ->setCategoryLabels(['a', 'b', 'c']),
    FastChart\BubbleChart::class => fn() => (new FastChart\BubbleChart(240, 180))
        ->setPoints([[1, 1, 10], [2, 3, 20], [3, 2, 15]]),
    FastChart\SurfaceChart::class => fn() => (new FastChart\SurfaceChart(240, 180))
        ->setGrid([[1, 2], [3, 4]]),
    FastChart\GaugeChart::class => fn() => (new FastChart\GaugeChart(240, 180))
        ->setValue(42),
    FastChart\GanttChart::class => fn() => (new FastChart\GanttChart(300, 180))
        ->setTasks([
            ['label' => 'a', 'start' => 0, 'end' => 5],
            ['label' => 'b', 'start' => 3, 'end' => 8],
        ]),
    FastChart\BoxPlot::class => fn() => (new FastChart\BoxPlot(240, 180))
        ->setBoxes([
            ['min' => 1, 'q1' => 2, 'median' => 3, 'q3' => 4, 'max' => 5],
        ]),
    FastChart\PolarChart::class => fn() => (new FastChart\PolarChart(220, 220))
        ->setSeries([['data' => [[0, 1], [45, 2], [90, 3]]]]),
    FastChart\ContourChart::class => fn() => (new FastChart\ContourChart(240, 180))
        ->setGrid([[1, 2], [3, 4]]),
    FastChart\Treemap::class => fn() => (new FastChart\Treemap(240, 180))
        ->setItems([
            ['label' => 'a', 'value' => 5],
            ['label' => 'b', 'value' => 3],
        ]),
    FastChart\Funnel::class => fn() => (new FastChart\Funnel(240, 180))
        ->setStages([
            ['label' => 'a', 'value' => 100],
            ['label' => 'b', 'value' => 50],
        ]),
    FastChart\Waterfall::class => fn() => (new FastChart\Waterfall(240, 180))
        ->setBars([
            ['label' => 'a', 'value' => 100],
            ['label' => 'b', 'value' => -20],
        ]),
    FastChart\Heatmap::class => fn() => (new FastChart\Heatmap(240, 180))
        ->setGrid([[1, 2], [3, 4]]),
    FastChart\LinearMeter::class => fn() => (new FastChart\LinearMeter(240, 80))
        ->setValue(40),
    FastChart\BulletChart::class => fn() => (new FastChart\BulletChart(300, 100))
        ->setRange(0, 100)
        ->setBands([
            ['from' => 0, 'to' => 60],
            ['from' => 60, 'to' => 100],
        ])
        ->setValue(72)
        ->setTarget(80),
    FastChart\ParetoChart::class => fn() => (new FastChart\ParetoChart(300, 220))
        ->setBars([
            ['label' => 'a', 'value' => 40],
            ['label' => 'b', 'value' => 20],
        ]),
    FastChart\CalendarHeatmap::class => fn() => (new FastChart\CalendarHeatmap(400, 140))
        ->setData(['2026-01-05' => 3, '2026-02-14' => 9]),
    FastChart\SunburstChart::class => fn() => (new FastChart\SunburstChart(220, 220))
        ->setHierarchy([
            'label' => 'root',
            'children' => [
                ['label' => 'a', 'value' => 10],
                ['label' => 'b', 'value' => 20],
            ],
        ]),
    FastChart\SankeyChart::class => fn() => (new FastChart\SankeyChart(300, 200))
        ->setNodes([['label' => 'a'], ['label' => 'b']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 3]]),
    FastChart\ArcDiagram::class => fn() => (new FastChart\ArcDiagram(300, 180))
        ->setNodes([['label' => 'a'], ['label' => 'b']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 3]]),
    FastChart\ChordDiagram::class => fn() => (new FastChart\ChordDiagram(220, 220))
        ->setNodes([['label' => 'a'], ['label' => 'b']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 3]]),
    FastChart\NetworkChart::class => fn() => (new FastChart\NetworkChart(220, 220))
        ->setNodes([['label' => 'a'], ['label' => 'b']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 3]]),
    FastChart\PopulationPyramid::class => fn() => (new FastChart\PopulationPyramid(240, 180))
        ->setCategories(['a', 'b'])
        ->setLeftSeries(['label' => 'L', 'data' => [1, 2]])
        ->setRightSeries(['label' => 'R', 'data' => [2, 1]]),
    FastChart\ViolinPlot::class => fn() => (new FastChart\ViolinPlot(240, 180))
        ->setGroups([
            ['label' => 'a', 'values' => [1, 2, 3, 2]],
        ]),
    FastChart\CirclePacking::class => fn() => (new FastChart\CirclePacking(220, 220))
        ->setHierarchy([
            'children' => [['value' => 5], ['value' => 3]],
        ]),
    FastChart\Dendrogram::class => fn() => (new FastChart\Dendrogram(220, 220))
        ->setHierarchy([
            'children' => [['value' => 5], ['value' => 3]],
        ]),
    FastChart\Partition::class => fn() => (new FastChart\Partition(220, 220))
        ->setHierarchy([
            'children' => [['value' => 5], ['value' => 3]],
        ]),
    FastChart\Pictogram::class => fn() => (new FastChart\Pictogram(240, 120))
        ->setTotal(10)
        ->setValue(6),
    FastChart\VennDiagram::class => fn() => (new FastChart\VennDiagram(220, 220))
        ->setSets([['size' => 10], ['size' => 8]])
        ->setIntersections([['sets' => [0, 1], 'size' => 3]]),
    FastChart\WordCloud::class => fn() => (new FastChart\WordCloud(240, 180))
        ->setWords([
            ['text' => 'alpha', 'weight' => 5],
            ['text' => 'beta', 'weight' => 3],
        ]),
    FastChart\SerpentineTimeline::class => fn() => (new FastChart\SerpentineTimeline(300, 180))
        ->setEvents([
            ['label' => 'a', 'date' => 'Jan'],
            ['label' => 'b', 'date' => 'Feb'],
        ]),
    FastChart\MarimekkoChart::class => fn() => (new FastChart\MarimekkoChart(300, 220))
        ->setColumns([
            ['label' => 'Q1', 'segments' => [
                ['label' => 'x', 'value' => 30],
                ['label' => 'y', 'value' => 20],
            ]],
            ['label' => 'Q2', 'segments' => [
                ['label' => 'x', 'value' => 40],
            ]],
        ]),
    FastChart\VectorChart::class => fn() => (new FastChart\VectorChart(220, 220))
        ->setVectors([
            ['x' => 0, 'y' => 0, 'dx' => 1, 'dy' => 1],
            ['x' => 1, 'y' => 0, 'dx' => -1, 'dy' => 1],
        ]),
    FastChart\Code128::class => fn() => (new FastChart\Code128())
        ->setData('FC-12345')
        ->setSize(240, 80),
    FastChart\QrCode::class => fn() => (new FastChart\QrCode())
        ->setData('https://example.com')
        ->setSize(180, 180),
];

$catalog = array_keys($builders);
sort($catalog);

$concrete = [];
foreach (get_declared_classes() as $class) {
    if (!str_starts_with($class, 'FastChart\\')) {
        continue;
    }

    $reflection = new ReflectionClass($class);
    if ($reflection->isInternal() && !$reflection->isAbstract()) {
        $concrete[] = $reflection->getName();
    }
}
sort($concrete);

$missing = array_values(array_diff($concrete, $catalog));
$extra = array_values(array_diff($catalog, $concrete));
$catalog_ok = count($builders) === 40 && $missing === [] && $extra === [];
echo 'catalog: ', $catalog_ok ? '40 concrete classes' : 'MISMATCH', "\n";
if (!$catalog_ok) {
    echo 'uncatalogued: ', implode(', ', $missing), "\n";
    echo 'non-concrete catalog entries: ', implode(', ', $extra), "\n";
}

foreach ($builders as $class => $builder) {
    $failures = [];

    try {
        $original = build_object($builder);
        if ($original::class !== $class) {
            $failures[] = 'builder-class:' . $original::class;
        }
        $baseline = $original->renderSvg();
        if (!valid_svg($baseline)) {
            $failures[] = 'baseline-svg';
        }

        $clone = clone $original;
        if ($clone->renderSvg() !== $baseline) {
            $failures[] = 'clone-identity';
        }

        mutate_base($clone, 0x123456);
        $mutated_clone = $clone->renderSvg();
        if ($mutated_clone === $baseline) {
            $failures[] = 'clone-mutation';
        }
        if ($original->renderSvg() !== $baseline) {
            $failures[] = 'clone-to-original-isolation';
        }

        unset($original);
        if ($clone->renderSvg() !== $mutated_clone) {
            $failures[] = 'clone-outlives-original';
        }

        $original = build_object($builder);
        $clone = clone $original;
        mutate_base($original, 0x654321);
        $mutated_original = $original->renderSvg();
        if ($mutated_original === $baseline) {
            $failures[] = 'original-mutation';
        }
        if ($clone->renderSvg() !== $baseline) {
            $failures[] = 'original-to-clone-isolation';
        }

        unset($clone);
        if ($original->renderSvg() !== $mutated_original) {
            $failures[] = 'original-outlives-clone';
        }
        if (!dynamic_property_blocked($original)) {
            $failures[] = 'dynamic-property';
        }
        if (!serialization_blocked($original)) {
            $failures[] = 'serialization';
        }
    } catch (Throwable $exception) {
        $failures[] = 'exception:' . $exception::class . ':' . $exception->getMessage();
    }

    echo (new ReflectionClass($class))->getShortName(), ': ',
        $failures === [] ? 'ok' : 'FAIL ' . implode(', ', $failures), "\n";
}

?>
--EXPECT--
catalog: 40 concrete classes
LineChart: ok
AreaChart: ok
BarChart: ok
PieChart: ok
ScatterChart: ok
StockChart: ok
RadarChart: ok
BubbleChart: ok
SurfaceChart: ok
GaugeChart: ok
GanttChart: ok
BoxPlot: ok
PolarChart: ok
ContourChart: ok
Treemap: ok
Funnel: ok
Waterfall: ok
Heatmap: ok
LinearMeter: ok
BulletChart: ok
ParetoChart: ok
CalendarHeatmap: ok
SunburstChart: ok
SankeyChart: ok
ArcDiagram: ok
ChordDiagram: ok
NetworkChart: ok
PopulationPyramid: ok
ViolinPlot: ok
CirclePacking: ok
Dendrogram: ok
Partition: ok
Pictogram: ok
VennDiagram: ok
WordCloud: ok
SerpentineTimeline: ok
MarimekkoChart: ok
VectorChart: ok
Code128: ok
QrCode: ok
