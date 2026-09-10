<?php
/* PHP 8.5 E_ALL render gate: exit 0 on success, 1 for deprecations, 2 for errors. */

error_reporting(E_ALL);

$deprecations = [];
set_error_handler(
    function (int $errno, string $errstr, string $errfile, int $errline)
        use (&$deprecations): bool {
        if ($errno === E_DEPRECATED || $errno === E_USER_DEPRECATED) {
            $deprecations[] = "$errstr ($errfile:$errline)";
            return true;
        }
        return false;
    }
);

try {
    $series = [1, 4, 2, 5, 3, 6];

    $charts = [
        (new FastChart\LineChart(320, 200))->setSeries($series),
        (new FastChart\AreaChart(320, 200))->setSeries($series),
        (new FastChart\BarChart(320, 200))->setSeries($series),
        (new FastChart\PieChart(320, 200))
            ->setSlices(['a' => 1, 'b' => 2, 'c' => 3]),
        (new FastChart\ScatterChart(320, 200))
            ->setPoints([[1, 1], [2, 3], [3, 2]]),
        (new FastChart\StockChart(400, 250))->setOhlcv([
            [1700000000, 100, 102, 99, 101, 1000],
            [1700086400, 101, 103, 100, 102, 1100],
        ]),
    ];

    foreach ($charts as $chart) {
        $chart->renderSvg();
        $chart->renderPng();
        $chart->renderJpeg();
        $chart->renderWebp();
        $chart->renderToFile(
            sys_get_temp_dir() . '/fc-deprecation-gate.svg');
        $chart->drawSvgFragment('gate');
        $chart->getImageMap();
    }

    $symbol = (new FastChart\QrCode())->setData('deprecation-gate');
    $symbol->renderSvg();
    $symbol->renderPng();
    $symbol->renderJpeg();
    $symbol->renderWebp();

    $svg = FastChart\Chart::svgToPng($charts[0]->renderSvg());
    if (!str_starts_with($svg, "\x89PNG")) {
        throw new RuntimeException('svgToPng round-trip broke');
    }
} catch (Throwable $e) {
    fwrite(STDERR, 'deprecation-gate ERROR: '
        . get_class($e) . ': ' . $e->getMessage() . "\n");
    exit(2);
} finally {
    @unlink(sys_get_temp_dir() . '/fc-deprecation-gate.svg');
}

if ($deprecations !== []) {
    foreach ($deprecations as $notice) {
        fwrite(STDERR, "deprecation-gate DEPRECATED: $notice\n");
    }
    exit(1);
}

echo "deprecation-gate clean: no E_DEPRECATED under E_ALL\n";
