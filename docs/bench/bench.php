<?php
/* fastchart benchmark.
 *
 * Runs each chart type at small (640x480) and large (1920x1080)
 * resolutions, times renderPng() (in-memory encode), and reports
 * median / p95 / ops-per-second.
 *
 * Run:
 *   php -d extension=gd -d extension=./modules/fastchart.so \
 *       docs/bench/bench.php
 *
 * Iteration counts via env:
 *   FC_BENCH_SMALL_ITERS=500 FC_BENCH_LARGE_ITERS=100
 *
 * Data shape stays constant across resolutions so the only
 * variable is canvas size.
 */

require __DIR__ . '/../examples/_bootstrap.php';

$N_SMALL = (int)(getenv('FC_BENCH_SMALL_ITERS') ?: 200);
$N_LARGE = (int)(getenv('FC_BENCH_LARGE_ITERS') ?: 50);
$WARMUP  = 3;

/* Deterministic data so runs are comparable. */
mt_srand(42);

$line_data = [];
for ($i = 0; $i < 100; $i++) {
    $line_data[] = 100 + sin($i * 0.15) * 30 + mt_rand(-10, 10);
}

$area_series = [
    ['label' => 'Free', 'data' => array_map(fn($i) => 300 + $i * 5,            range(0, 49))],
    ['label' => 'Pro',  'data' => array_map(fn($i) => 100 + $i * 6,            range(0, 49))],
    ['label' => 'Team', 'data' => array_map(fn($i) => 30  + $i * 4,            range(0, 49))],
];

$bar_series = [
    ['label' => 'Opened', 'data' => [42, 38, 51, 47, 55, 49, 52, 60, 58, 64, 61, 70]],
    ['label' => 'Closed', 'data' => [35, 41, 48, 50, 52, 54, 49, 55, 60, 58, 62, 65]],
];
$bar_labels = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];

$scatter_pts = [];
for ($i = 0; $i < 200; $i++) {
    $x = $i * 0.5 + mt_rand(-100, 100) / 100.0;
    $y = 0.04 * $x * $x + 0.5 * $x + 5 + mt_rand(-200, 200) / 100.0;
    $scatter_pts[] = [$x, $y];
}

$bubble_pts = [];
for ($i = 0; $i < 50; $i++) {
    $bubble_pts[] = [mt_rand(0, 100), mt_rand(0, 100), mt_rand(3, 25)];
}

$ohlcv = [];
$base_ts = 1700000000;
$price = 100.0;
for ($i = 0; $i < 90; $i++) {
    $open  = $price;
    $close = $price + (mt_rand(-100, 100) / 100.0) * 1.8;
    $high  = max($open, $close) + mt_rand(0, 80) / 100.0;
    $low   = min($open, $close) - mt_rand(0, 80) / 100.0;
    $vol   = 50000 + mt_rand(0, 30000);
    $ohlcv[] = [$base_ts + $i * 86400, $open, $high, $low, $close, $vol];
    $price = $close;
}

$polar_a = [];
$polar_b = [];
for ($a = 0; $a <= 360; $a += 15) {
    $polar_a[] = [$a, 4 + 3 * sin(deg2rad($a) * 2)];
    $polar_b[] = [$a, 3 + 2 * cos(deg2rad($a))];
}

$grid = [];
for ($r = 0; $r < 16; $r++) {
    $row = [];
    for ($c = 0; $c < 24; $c++) {
        $row[] = sin($c / 4.0) * cos($r / 3.0) * 10.0;
    }
    $grid[] = $row;
}

$heat_grid = [];
for ($d = 0; $d < 7; $d++) {
    $row = [];
    for ($h = 0; $h < 24; $h++) {
        $weekday = $d < 5 ? 1 : 0;
        $morning = exp(-pow($h - 9, 2) / 12);
        $evening = exp(-pow($h - 21, 2) / 8);
        $base = 20 + 80 * $weekday * $morning + 60 * (1 - $weekday) * $evening;
        $row[] = $base + sin($d + $h) * 4;
    }
    $heat_grid[] = $row;
}

$day = 86400;

/* Builder closures: each takes (width, height), returns a chart
 * configured but not yet rendered. */
$builders = [
    'LineChart' => fn($w, $h) =>
        (new FastChart\LineChart($w, $h))
            ->setFontPath($font)
            ->setTitle('Daily active users')
            ->setSeries([['data' => $line_data]]),

    'AreaChart' => fn($w, $h) =>
        (new FastChart\AreaChart($w, $h))
            ->setFontPath($font)
            ->setTitle('Active users by tier')
            ->setSeries($area_series)
            ->setStacked(true)
            ->setFillOpacity(110),

    'BarChart' => fn($w, $h) =>
        (new FastChart\BarChart($w, $h))
            ->setFontPath($font)
            ->setTitle('Tickets opened vs closed')
            ->setSeries($bar_series)
            ->setCategoryLabels($bar_labels)
            ->setShowValues(true),

    'ScatterChart' => fn($w, $h) =>
        (new FastChart\ScatterChart($w, $h))
            ->setFontPath($font)
            ->setTitle('Page load vs payload')
            ->setPoints($scatter_pts)
            ->setMarkerStyle(FastChart\Chart::MARKER_CIRCLE)
            ->setMarkerSize(5)
            ->setTrendLine(true, 0xE34A6F, 2),

    'BubbleChart' => fn($w, $h) =>
        (new FastChart\BubbleChart($w, $h))
            ->setFontPath($font)
            ->setTitle('Effort vs risk')
            ->setPoints($bubble_pts),

    'PieChart' => fn($w, $h) =>
        (new FastChart\PieChart($w, $h))
            ->setFontPath($font)
            ->setTitle('Traffic sources')
            ->setSlices([
                'Organic search' => 41,
                'Direct'         => 28,
                'Referral'       => 15,
                'Social'         => 9,
                'Email'          => 5,
                'Paid'           => 2,
            ])
            ->setDonutHoleRatio(0.5)
            ->setSliceLabelPosition(FastChart\Chart::LABEL_OUTSIDE)
            ->setSliceLabelFormat('%.0f%%'),

    'StockChart' => fn($w, $h) =>
        (new FastChart\StockChart($w, $h))
            ->setFontPath($font)
            ->setTitle('ACME 90d OHLCV')
            ->setOhlcv($ohlcv)
            ->addMovingAverage(10, FastChart\StockChart::MA_SMA)
            ->addMovingAverage(20, FastChart\StockChart::MA_EMA)
            ->addMovingAverage(30, FastChart\StockChart::MA_WMA)
            ->setVolumePane(true)
            ->setCandleStyle(FastChart\Chart::STYLE_CANDLE),

    'RadarChart' => fn($w, $h) =>
        (new FastChart\RadarChart($w, $h))
            ->setFontPath($font)
            ->setTitle('Feature parity')
            ->setSeries([
                ['label' => 'A', 'data' => [8, 7, 9, 6, 8, 7]],
                ['label' => 'B', 'data' => [6, 9, 7, 8, 5, 9]],
            ])
            ->setCategoryLabels(['Speed','Reliability','API','Docs','Support','Pricing'])
            ->setMaxValue(10),

    'PolarChart' => fn($w, $h) =>
        (new FastChart\PolarChart($w, $h))
            ->setFontPath($font)
            ->setTitle('Antenna gain')
            ->setSeries([
                ['label' => 'A', 'data' => $polar_a],
                ['label' => 'B', 'data' => $polar_b],
            ])
            ->setMaxRadius(8),

    'SurfaceChart' => fn($w, $h) =>
        (new FastChart\SurfaceChart($w, $h))
            ->setFontPath($font)
            ->setTitle('Surface heatmap')
            ->setGrid($grid)
            ->setColorRamp(0x1F4E79, 0xE34A6F),

    'ContourChart' => fn($w, $h) =>
        (new FastChart\ContourChart($w, $h))
            ->setFontPath($font)
            ->setTitle('Filled contour')
            ->setGrid($grid)
            ->setLevels([-8, -4, -2, 0, 2, 4, 8])
            ->setFilled(true)
            ->setColorRamp(0x1F4E79, 0xE34A6F),

    'GaugeChart' => fn($w, $h) =>
        (new FastChart\GaugeChart($w, $h))
            ->setFontPath($font)
            ->setTitle('CPU')
            ->setRange(0, 100)
            ->setValue(72)
            ->setZones([
                ['from' => 0,  'to' => 50,  'color' => 0x4FB286],
                ['from' => 50, 'to' => 80,  'color' => 0xF6AE2D],
                ['from' => 80, 'to' => 100, 'color' => 0xE34A6F],
            ])
            ->setValueFormat('%.0f%%'),

    'GanttChart' => fn($w, $h) =>
        (new FastChart\GanttChart($w, $h))
            ->setFontPath($font)
            ->setTitle('Q4 release')
            ->setTimeRange($base_ts, $base_ts + 60 * $day)
            ->setTasks([
                ['name' => 'Spec',   'start' => $base_ts + 0  * $day, 'end' => $base_ts + 7  * $day],
                ['name' => 'Design', 'start' => $base_ts + 5  * $day, 'end' => $base_ts + 18 * $day, 'depends' => [0]],
                ['name' => 'Build',  'start' => $base_ts + 15 * $day, 'end' => $base_ts + 38 * $day, 'depends' => [1]],
                ['name' => 'QA',     'start' => $base_ts + 35 * $day, 'end' => $base_ts + 50 * $day, 'depends' => [2]],
                ['name' => 'Launch', 'start' => $base_ts + 50 * $day, 'end' => $base_ts + 50 * $day, 'milestone' => true],
            ])
            ->setShowTaskLabels(true),

    'BoxPlot' => fn($w, $h) =>
        (new FastChart\BoxPlot($w, $h))
            ->setFontPath($font)
            ->setTitle('Latency by service')
            ->setBoxes([
                ['label' => 'auth',    'min' => 18, 'q1' => 32, 'median' => 45,  'q3' => 58,  'max' => 95,  'outliers' => [120, 135]],
                ['label' => 'profile', 'min' => 42, 'q1' => 65, 'median' => 82,  'q3' => 105, 'max' => 160, 'outliers' => [220]],
                ['label' => 'search',  'min' => 80, 'q1' => 110,'median' => 145, 'q3' => 195, 'max' => 320, 'outliers' => [410, 480, 510]],
                ['label' => 'orders',  'min' => 25, 'q1' => 38, 'median' => 52,  'q3' => 71,  'max' => 105],
            ]),

    'Treemap' => fn($w, $h) =>
        (new FastChart\Treemap($w, $h))
            ->setFontPath($font)
            ->setTitle('Q3 revenue')
            ->setItems([
                ['label' => 'Cloud',     'value' => 4200],
                ['label' => 'Hardware',  'value' => 2900],
                ['label' => 'Services',  'value' => 2100],
                ['label' => 'Licenses',  'value' => 1500],
                ['label' => 'Training',  'value' => 900],
                ['label' => 'Support',   'value' => 700],
                ['label' => 'Marketing', 'value' => 400],
                ['label' => 'Other',     'value' => 300],
            ]),

    'Funnel' => fn($w, $h) =>
        (new FastChart\Funnel($w, $h))
            ->setFontPath($font)
            ->setTitle('Signup to paid')
            ->setStages([
                ['label' => 'Visit',   'value' => 12_400],
                ['label' => 'Sign up', 'value' =>  3_200],
                ['label' => 'Active',  'value' =>  1_950],
                ['label' => 'Trial',   'value' =>    820],
                ['label' => 'Paid',    'value' =>    310],
            ]),

    'Waterfall' => fn($w, $h) =>
        (new FastChart\Waterfall($w, $h))
            ->setFontPath($font)
            ->setTitle('Q3 income')
            ->setBars([
                ['label' => 'Revenue', 'value' => 1200, 'kind' => 'total'],
                ['label' => 'COGS',    'value' => -480],
                ['label' => 'Margin',  'value' => 720,  'kind' => 'total'],
                ['label' => 'R&D',     'value' => -180],
                ['label' => 'S&M',     'value' => -220],
                ['label' => 'G&A',     'value' => -90],
                ['label' => 'OpInc',   'value' => 230,  'kind' => 'total'],
            ]),

    'Heatmap' => fn($w, $h) =>
        (new FastChart\Heatmap($w, $h))
            ->setFontPath($font)
            ->setTitle('Hourly traffic')
            ->setGrid($heat_grid)
            ->setColorRamp(0xF1F5F9, 0xE34A6F),

    'LinearMeter' => fn($w, $h) =>
        (new FastChart\LinearMeter($w, $h))
            ->setFontPath($font)
            ->setTitle('CPU')
            ->setRange(0, 100)
            ->setValue(72)
            ->setZones([
                ['from' => 0,  'to' => 60,  'color' => 0x4FB286],
                ['from' => 60, 'to' => 85,  'color' => 0xE8A33D],
                ['from' => 85, 'to' => 100, 'color' => 0xE34A6F],
            ])
            ->setValueFormat('%.0f%%'),
];

$resolutions = [
    'small (640x480)'  => [640,  480,  $N_SMALL],
    'large (1920x1080)' => [1920, 1080, $N_LARGE],
];

$php_v = PHP_VERSION;
$ext_v = FastChart\Chart::version();
printf("# fastchart bench  ext=%s  php=%s\n", $ext_v, $php_v);
printf("# warmup=%d  small_iters=%d  large_iters=%d\n\n",
    $WARMUP, $N_SMALL, $N_LARGE);

printf("%-14s %-18s %10s %10s %12s %12s\n",
    'chart', 'resolution', 'median_ms', 'p95_ms', 'ops_per_sec', 'png_bytes');
printf("%s\n", str_repeat('-', 80));

foreach ($builders as $name => $build) {
    foreach ($resolutions as $reso_name => [$w, $h, $N]) {
        for ($i = 0; $i < $WARMUP; $i++) {
            $build($w, $h)->renderPng();
        }

        $samples = [];
        $bytes = 0;
        for ($i = 0; $i < $N; $i++) {
            $t0 = hrtime(true);
            $png = $build($w, $h)->renderPng();
            $samples[] = (hrtime(true) - $t0) / 1e6;
            if ($i === 0) $bytes = strlen($png);
        }
        sort($samples);
        $median = $samples[(int)floor(count($samples) * 0.5)];
        $p95    = $samples[(int)floor(count($samples) * 0.95)];
        $ops    = 1000.0 / $median;

        printf("%-14s %-18s %10.2f %10.2f %12.1f %12d\n",
            $name, $reso_name, $median, $p95, $ops, $bytes);
    }
}

printf("\n# done\n");
