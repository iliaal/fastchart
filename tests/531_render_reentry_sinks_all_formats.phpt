--TEST--
Render-time callbacks cannot free chart data the renderer holds, in any output format
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

final class SinkPngStream
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
        $hook = self::$hook;
        self::$hook = null;
        if ($hook) {
            $hook();
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
stream_wrapper_register('fcsink', SinkPngStream::class);

function attempt(Closure $mutation): string
{
    try {
        $mutation();
        return 'MUTATED';
    } catch (Error $e) {
        return str_contains($e->getMessage(), 'while it is rendering')
            || str_contains($e->getMessage(), 'already rendering')
            ? 'refused' : get_class($e) . ': ' . $e->getMessage();
    }
}

$tmp = sys_get_temp_dir() . '/fc_sink_' . getmypid();
$formats = [
    'svg' => fn($c) => str_contains($c->renderSvg(), '</svg>'),
    'fragment' => fn($c) => str_contains($c->drawSvgFragment('f'), '</g>'),
    'png' => fn($c) => str_starts_with($c->renderPng(), "\x89PNG"),
    'jpeg' => fn($c) => str_starts_with($c->renderJpeg(), "\xFF\xD8"),
    'webp' => fn($c) => str_starts_with($c->renderWebp(), 'RIFF'),
    'svg-file' => function ($c) use ($tmp) {
        $ok = $c->renderToFile("$tmp.svg") > 0;
        @unlink("$tmp.svg");
        return $ok;
    },
    'png-file' => function ($c) use ($tmp) {
        $ok = $c->renderToFile("$tmp.png") > 0;
        @unlink("$tmp.png");
        return $ok;
    },
];
try {
    (new FastChart\LineChart(10, 10))->setSeries([1])->renderPdf();
    $formats['pdf'] = fn($c) => str_starts_with($c->renderPdf(), '%PDF');
    $formats['pdf-file'] = function ($c) use ($tmp) {
        $ok = $c->renderToFile("$tmp.pdf") > 0;
        @unlink("$tmp.pdf");
        return $ok;
    };
} catch (Error) {
}

$alt_font = fc_pick_font();
$sinks = [
    'icons' => [
        fn() => (new FastChart\LineChart(300, 200))->setSeries([1, 2, 3, 4])
            ->addIconAt(0, 1, 'fcsink://a.png', 10, 10),
        function ($c) {
            for ($i = 0; $i < 20; $i++) {
                $c->addIconAt(0, 1, 'fcsink://b.png');
            }
        },
    ],
    'points' => [
        fn() => (new FastChart\ScatterChart(300, 200))
            ->setPoints(array_map(fn($i) => [$i, $i % 7], range(0, 50)))
            ->setBackgroundImage('fcsink://bg.png'),
        fn($c) => $c->setPoints([[1, 1]]),
    ],
    'bars-count' => [
        fn() => (new FastChart\Waterfall(300, 200))
            ->setBars(array_map(fn($i) => ['label' => "b$i", 'value' => $i + 1], range(0, 63)))
            ->setBackgroundImage('fcsink://bg.png'),
        fn($c) => $c->setBars([['value' => 1]]),
    ],
    'category-labels' => [
        fn() => (new FastChart\BoxPlot(300, 200))
            ->setBoxes(array_fill(0, 6, [1, 2, 3, 4, 5]))
            ->setCategoryLabels(['aaaaaaaa', 'bbbbbbbb', 'cccccccc', 'dddddddd', 'eeeeeeee', 'ffffffff'])
            ->setBackgroundImage('fcsink://bg.png'),
        fn($c) => $c->setCategoryLabels([]),
    ],
    'font-path' => [
        fn() => (new FastChart\LineChart(300, 200))->setSeries([1, 2, 3])
            ->setAxisFont(implode('', [$alt_font, '']), 11.0)
            ->setBackgroundImage('fcsink://bg.png'),
        fn($c) => $c->setAxisFont($GLOBALS['alt_font'], 12.0),
    ],
    'background-path' => [
        fn() => (new FastChart\LineChart(200, 100))->setSeries([1, 2, 3])
            ->setBackgroundImage('fcsink://' . str_repeat('A', 40) . '.png'),
        function ($c) {
            $c->setBackgroundImage('fcsink://second.png');
        },
    ],
    'nested-render' => [
        fn() => (new FastChart\LineChart(200, 100))->setSeries([1, 2, 3])
            ->setBackgroundImage('fcsink://bg.png'),
        fn($c) => $c->renderSvg(),
    ],
];

foreach ($sinks as $name => [$build, $mutation]) {
    foreach ($formats as $format => $render) {
        $chart = $build();
        $outcome = 'not reached';
        SinkPngStream::$hook = function () use ($chart, $mutation, &$outcome) {
            $outcome = attempt(fn() => $mutation($chart));
        };
        $ok = $render($chart);
        if ($outcome !== 'refused' || !$ok) {
            echo "$name/$format: $outcome, output ", $ok ? 'ok' : 'BAD', "\n";
        }
    }
}

/* Error handlers are the other route into user code: an unknown scheme
 * warns from inside the render even with no wrapper registered. */
$chart = (new FastChart\ScatterChart(300, 200))
    ->setPoints(array_map(fn($i) => [$i, $i % 7], range(0, 50)))
    ->setBackgroundImage('fcunknownscheme://x.png');
$outcome = 'not reached';
set_error_handler(function () use ($chart, &$outcome) {
    if ($outcome === 'not reached') {
        $outcome = attempt(fn() => $chart->setPoints([[1, 1]]));
    }
    return true;
});
$ok = str_contains($chart->renderSvg(), '</svg>');
restore_error_handler();
echo "error-handler: $outcome, output ", $ok ? 'ok' : 'BAD', "\n";

/* A clone taken mid-render is an independent, idle chart. */
$chart = (new FastChart\LineChart(200, 100))->setSeries([1, 2, 3])
    ->setBackgroundImage('fcsink://bg.png');
$clone_result = 'not reached';
SinkPngStream::$hook = function () use ($chart, &$clone_result) {
    $copy = clone $chart;
    $copy->setBackgroundImage('')->setTitle('copy');
    $clone_result = str_contains($copy->renderSvg(), '</svg>')
        ? 'idle' : 'BAD';
};
$chart->renderSvg();
echo "clone: $clone_result\n";
$chart->setTitle('after');
echo "after render: ", str_contains($chart->renderSvg(), '</svg>') ? 'mutable' : 'BAD', "\n";
?>
--EXPECT--
error-handler: refused, output ok
clone: idle
after render: mutable
