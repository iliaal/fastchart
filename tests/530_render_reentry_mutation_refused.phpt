--TEST--
Every mutating chart method refuses while the chart is rendering
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die("skip no system font available\n");
?>
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';

final class ReentryPngStream
{
    public static ?Closure $hook = null;
    public $context;
    private int $position = 0;
    private string $bytes;

    public function stream_open(string $path, string $mode): bool
    {
        $this->bytes = base64_decode(
            'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII='
        );
        if (self::$hook) {
            (self::$hook)();
        }
        return true;
    }

    public function stream_read(int $count): string
    {
        $chunk = substr($this->bytes, $this->position, $count);
        $this->position += strlen($chunk);
        return $chunk;
    }

    public function stream_eof(): bool
    {
        return $this->position >= strlen($this->bytes);
    }

    public function stream_stat(): array
    {
        return ['mode' => 0100000, 'size' => strlen($this->bytes)];
    }
}
stream_wrapper_register('fcreentry', ReentryPngStream::class);

const READERS = [
    'renderSvg', 'renderPng', 'renderJpeg', 'renderWebp', 'renderPdf',
    'renderToFile', 'drawSvgFragment', 'getImageMap', 'getImageMapAreas',
];

/* Calls every public instance method except the renderers and image-map
 * readers with no arguments; the guard must fire before argument parsing. */
function probe(object $chart): array
{
    $unguarded = [];
    $checked = 0;
    foreach ((new ReflectionClass($chart))->getMethods() as $m) {
        if (!$m->isPublic() || $m->isStatic()
            || in_array($m->getName(), READERS, true)) {
            continue;
        }
        $checked++;
        try {
            $m->invoke($chart);
            $unguarded[] = $m->getName() . ':returned';
        } catch (Throwable $e) {
            if (!$e instanceof Error || $e instanceof ArgumentCountError
                || !str_contains($e->getMessage(),
                    'cannot modify a chart while it is rendering')) {
                $unguarded[] = $m->getName() . ':' . $e::class;
            }
        }
    }
    foreach (['renderSvg', 'renderPng', 'drawSvgFragment'] as $r) {
        try {
            $chart->$r();
            $unguarded[] = $r . ':nested-render-returned';
        } catch (Error $e) {
            if (!str_contains($e->getMessage(), 'already rendering')) {
                $unguarded[] = $r . ':' . $e->getMessage();
            }
        }
    }
    return [$checked, $unguarded];
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
];

/* WordCloud never paints the background image; its render reaches user
 * code through a font path that looks like a URL and no longer resolves
 * locally, which makes the font loader consult the stream wrappers and
 * warn into the error handler. Windows has no such route: a colon cannot
 * appear in a file name there, and a missing local font fails silently. */
$expected_classes = count($builders);
if (PHP_OS_FAMILY === 'Windows') {
    unset($builders[FastChart\WordCloud::class]);
}
$font = fc_pick_font();
$dir = sys_get_temp_dir() . '/fc_reentry_' . getmypid();
@mkdir($dir);
if (PHP_OS_FAMILY !== 'Windows') {
    @mkdir($dir . '/fcnoscheme:');
    copy($font, $dir . '/fcnoscheme:/f.ttf');
}
$cwd = getcwd();
chdir($dir);

$failures = 0;
$classes = 0;
$methods = 0;
foreach ($builders as $class => $builder) {
    $chart = $builder();
    $chart->setSvgTextMode(0);
    $result = null;
    $fired = 0;
    $hook = function () use ($chart, &$result, &$fired) {
        if ($fired++ === 0) {
            $result = probe($chart);
        }
    };
    if ($class === FastChart\WordCloud::class) {
        $chart->setFontPath('fcnoscheme://f.ttf');
        unlink($dir . '/fcnoscheme:/f.ttf');
        set_error_handler(function () use ($hook) {
            $hook();
            return true;
        });
    } else {
        $chart->setBackgroundImage('fcreentry://bg.png');
        ReentryPngStream::$hook = $hook;
    }
    $svg = $chart->renderSvg();
    ReentryPngStream::$hook = null;
    restore_error_handler();
    $classes++;
    if ($result === null) {
        echo "$class: render never reached user code\n";
        $failures++;
        continue;
    }
    [$checked, $unguarded] = $result;
    $methods += $checked;
    if ($unguarded !== [] || !str_contains($svg, '</svg>')) {
        echo "$class: ", implode(', ', $unguarded), "\n";
        $failures++;
    }
    /* The flag is lowered once the render returns. */
    $chart->setTitle('after');
    if (!str_contains($chart->renderSvg(), '</svg>')) {
        echo "$class: render after re-entry failed\n";
        $failures++;
    }
}
chdir($cwd);
@rmdir($dir . '/fcnoscheme:');
@rmdir($dir);
$want = PHP_OS_FAMILY === 'Windows' ? $expected_classes - 1 : $expected_classes;
echo $expected_classes === 38 && $classes === $want
    ? "classes: all\n" : "classes: $classes of $expected_classes\n";
echo $methods > $classes * 40
    ? "methods: many\n" : "methods: too few ($methods)\n";
echo "failures: $failures\n";
?>
--EXPECT--
classes: all
methods: many
failures: 0
