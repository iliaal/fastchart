<?php
/* Canonical gallery case list. Required by both
 * scripts/build-readme-gallery.php (which prints summary stats) and
 * scripts/build-v1-gallery.php (which emits the four-up SVG/PNG/JPG/WebP
 * HTML at docs/v1-gallery.html). Previously build-v1-gallery.php
 * eval()'d a sliced copy of build-readme-gallery.php's prefix to share
 * $cases — that's now this file, returned as an array. Keeps eval()
 * out of the build pipeline and gives one canonical place to add a
 * new chart case.
 *
 * Returns: ['font' => string, 'dpi' => int, 'cases' => array].
 * Caller must have fastchart.so + ext/gd loaded — every case's build
 * closure constructs a FastChart\* instance.
 *
 * Font picking shares the tests/_font_candidates.inc.php helper so
 * the example bootstrap, the test suite, and the gallery generators
 * all probe the same distro paths. */

require_once __DIR__ . '/../tests/_font_candidates.inc.php';

$font = fc_pick_font();
$dpi  = 200;

/* Each case carries a label, a PHP source string for display, and a
 * build closure that returns the configured chart. Sources mirror
 * the corresponding docs/examples/*.php construction so callers can
 * cross-reference. */
$cases = [];

$cases[] = [
    'label' => '1. LineChart — daily active users',
    'ref'   => 'docs/examples/01_line_basic.php',
    'code'  => <<<'PHP'
(new FastChart\LineChart(640, 320))
    ->setTitle('Daily active users')
    ->setSeries([['data' => [820, 940, 870, 1020, 1180, 1250, 1340]]])
    ->setCategoryLabels(['Mon','Tue','Wed','Thu','Fri','Sat','Sun']);
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\LineChart(640, 320))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Daily active users')
            ->setSeries([['data' => [820, 940, 870, 1020, 1180, 1250, 1340]]])
            ->setCategoryLabels(['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun']);
    },
];

$cases[] = [
    'label' => '2. AreaChart — stacked tiers',
    'ref'   => 'docs/examples/27_area_stacked.php',
    'code'  => <<<'PHP'
$data = [
    ['label' => 'Free',  'data' => [320, 380, 410, 470, 520, 590, 640, 700]],
    ['label' => 'Pro',   'data' => [120, 150, 180, 210, 250, 290, 340, 400]],
    ['label' => 'Team',  'data' => [ 40,  55,  70,  95, 130, 170, 210, 260]],
];
(new FastChart\AreaChart(640, 320))
    ->setTitle('Active users by tier (stacked)')
    ->setSeries($data)
    ->setCategoryLabels(['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug'])
    ->setStacked(true)
    ->setFillOpacity(110);
PHP,
    'build' => function () use ($font, $dpi) {
        $data = [
            ['label' => 'Free',  'data' => [320, 380, 410, 470, 520, 590, 640, 700]],
            ['label' => 'Pro',   'data' => [120, 150, 180, 210, 250, 290, 340, 400]],
            ['label' => 'Team',  'data' => [ 40,  55,  70,  95, 130, 170, 210, 260]],
        ];
        return (new FastChart\AreaChart(640, 320))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Active users by tier (stacked)')
            ->setSeries($data)
            ->setCategoryLabels(['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug'])
            ->setStacked(true)
            ->setFillOpacity(110);
    },
];

$cases[] = [
    'label' => '3. BarChart — grouped (tickets opened vs closed)',
    'ref'   => 'docs/examples/03_bar_grouped.php',
    'code'  => <<<'PHP'
(new FastChart\BarChart(640, 380))
    ->setTitle('Monthly tickets opened vs closed')
    ->setSeries([
        ['label' => 'Opened', 'data' => [42, 38, 51, 47, 55, 49]],
        ['label' => 'Closed', 'data' => [35, 41, 48, 50, 52, 54]],
    ])
    ->setCategoryLabels(['Jan','Feb','Mar','Apr','May','Jun'])
    ->setSeriesColors([0xE34A6F, 0x4FB286])
    ->setShowValues(true)
    ->setEdgeColor(0x222222)
    ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT);
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\BarChart(640, 380))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Monthly tickets opened vs closed')
            ->setSeries([
                ['label' => 'Opened', 'data' => [42, 38, 51, 47, 55, 49]],
                ['label' => 'Closed', 'data' => [35, 41, 48, 50, 52, 54]],
            ])
            ->setCategoryLabels(['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun'])
            ->setSeriesColors([0xE34A6F, 0x4FB286])
            ->setShowValues(true)
            ->setEdgeColor(0x222222)
            ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT);
    },
];

$cases[] = [
    'label' => '4a. BarChart — horizontal, stacked, with SLA bands',
    'ref'   => 'docs/examples/04_bar_horizontal.php',
    'code'  => <<<'PHP'
(new FastChart\BarChart(680, 380))
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setTitle('p95 latency by endpoint (ms)')
    ->setSeries([
        ['label' => 'Read',  'data' => [120, 85, 240, 180, 95, 310, 145]],
        ['label' => 'Write', 'data' => [180, 120, 380, 290, 140, 480, 220]],
    ])
    ->setCategoryLabels(['users','sessions','reports','search','auth','exports','health'])
    ->setStacked(true)
    ->addVerticalBand(0, 200, 0x4FB286, 96, 'SLA')
    ->addVerticalBand(200, 500, 0xE34A6F, 96, 'over SLA');
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\BarChart(680, 380))
            ->setFontPath($font)->setDpi($dpi)
            ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
            ->setTitle('p95 latency by endpoint (ms)')
            ->setSeries([
                ['label' => 'Read',  'data' => [120, 85, 240, 180, 95, 310, 145]],
                ['label' => 'Write', 'data' => [180, 120, 380, 290, 140, 480, 220]],
            ])
            ->setCategoryLabels(['users', 'sessions', 'reports', 'search', 'auth', 'exports', 'health'])
            ->setStacked(true)
            ->addVerticalBand(0, 200, 0x4FB286, 96, 'SLA')
            ->addVerticalBand(200, 500, 0xE34A6F, 96, 'over SLA');
    },
];

$cases[] = [
    'label' => '4b. BarChart — floating bars (forecast salary band)',
    'ref'   => 'docs/examples/28_bar_variants.php',
    'code'  => <<<'PHP'
(new FastChart\BarChart(640, 320))
    ->setTitle('Floating bars: forecast salary range by role')
    ->setFloating(true)
    ->setSeries([
        [80,  120], [110, 160], [140, 220],
        [180, 280], [220, 350],
    ])
    ->setCategoryLabels(['Junior', 'Mid', 'Senior', 'Staff', 'Principal'])
    ->setSeriesColors([0x4F86C6])
    ->setEdgeColor(0x222222)
    ->setYAxisTitle('Salary ($K)');
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\BarChart(640, 320))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Floating bars: forecast salary range by role')
            ->setFloating(true)
            ->setSeries([
                [80,  120], [110, 160], [140, 220],
                [180, 280], [220, 350],
            ])
            ->setCategoryLabels(['Junior', 'Mid', 'Senior', 'Staff', 'Principal'])
            ->setSeriesColors([0x4F86C6])
            ->setEdgeColor(0x222222)
            ->setYAxisTitle('Salary ($K)');
    },
];

$cases[] = [
    'label' => '4c. BarChart — layered stack (visual envelope)',
    'ref'   => 'docs/examples/28_bar_variants.php',
    'code'  => <<<'PHP'
(new FastChart\BarChart(640, 320))
    ->setTitle('Layered stack: visual envelope of multiple series')
    ->setSeries([
        ['label' => 'Q1', 'data' => [42, 38, 51, 47, 55, 49]],
        ['label' => 'Q2', 'data' => [35, 41, 48, 50, 52, 54]],
        ['label' => 'Q3', 'data' => [28, 33, 39, 42, 47, 50]],
    ])
    ->setCategoryLabels(['Jan','Feb','Mar','Apr','May','Jun'])
    ->setStacked(true)
    ->setStackMode(FastChart\Chart::STACK_LAYER);
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\BarChart(640, 320))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Layered stack: visual envelope of multiple series')
            ->setSeries([
                ['label' => 'Q1', 'data' => [42, 38, 51, 47, 55, 49]],
                ['label' => 'Q2', 'data' => [35, 41, 48, 50, 52, 54]],
                ['label' => 'Q3', 'data' => [28, 33, 39, 42, 47, 50]],
            ])
            ->setCategoryLabels(['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun'])
            ->setStacked(true)
            ->setStackMode(FastChart\Chart::STACK_LAYER);
    },
];

$cases[] = [
    'label' => '4. ScatterChart — with quadratic trend',
    'ref'   => 'docs/examples/06_scatter_trend.php',
    'code'  => <<<'PHP'
$pts = [];
mt_srand(42);
for ($i = 0; $i < 60; $i++) {
    $x = $i * 0.5 + mt_rand(-100, 100) / 100.0;
    $y = 0.04 * $x * $x + 0.5 * $x + 5 + mt_rand(-200, 200) / 100.0;
    $pts[] = [$x, $y];
}
(new FastChart\ScatterChart(640, 400))
    ->setTitle('Page load time vs payload size')
    ->setXAxisTitle('Payload (KB)')
    ->setYAxisTitle('Time (s)')
    ->setPoints($pts)
    ->setMarkerStyle(FastChart\Chart::MARKER_CIRCLE)
    ->setMarkerSize(6)
    ->setTrendLine(true, 0xE34A6F, 2);
PHP,
    'build' => function () use ($font, $dpi) {
        mt_srand(42);
        $pts = [];
        for ($i = 0; $i < 60; $i++) {
            $x = $i * 0.5 + mt_rand(-100, 100) / 100.0;
            $y = 0.04 * $x * $x + 0.5 * $x + 5 + mt_rand(-200, 200) / 100.0;
            $pts[] = [$x, $y];
        }
        return (new FastChart\ScatterChart(640, 400))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Page load time vs payload size')
            ->setXAxisTitle('Payload (KB)')
            ->setYAxisTitle('Time (s)')
            ->setPoints($pts)
            ->setMarkerStyle(FastChart\Chart::MARKER_CIRCLE)
            ->setMarkerSize(6)
            ->setTrendLine(true, 0xE34A6F, 2);
    },
];

$cases[] = [
    'label' => '5. BubbleChart — risk vs effort vs cost',
    'ref'   => 'docs/examples/14_bubble.php',
    'code'  => <<<'PHP'
(new FastChart\BubbleChart(640, 400))
    ->setTitle('Project size vs delivery risk')
    ->setXAxisTitle('Estimated effort (story points)')
    ->setYAxisTitle('Risk score')
    ->setPoints([
        [8,  2.5, 5], [13, 1.8, 8],  [21, 3.2, 12],
        [34, 5.5, 18], [55, 7.2, 25], [89, 8.9, 40],
        [21, 4.0, 9],  [34, 6.3, 15], [13, 2.8, 4],
    ]);
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\BubbleChart(640, 400))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Project size vs delivery risk')
            ->setXAxisTitle('Estimated effort (story points)')
            ->setYAxisTitle('Risk score')
            ->setPoints([
                [8,  2.5, 5], [13, 1.8, 8],  [21, 3.2, 12],
                [34, 5.5, 18], [55, 7.2, 25], [89, 8.9, 40],
                [21, 4.0, 9],  [34, 6.3, 15], [13, 2.8, 4],
            ]);
    },
];

$cases[] = [
    'label' => '6. StockChart — OHLCV + MA overlays + volume',
    'ref'   => 'docs/examples/07_stock_candle_ma.php',
    'code'  => <<<'PHP'
mt_srand(1);
$rows = []; $base = 1700000000; $price = 100.0;
for ($i = 0; $i < 90; $i++) {
    $open  = $price;
    $close = $price + (mt_rand(-100, 100) / 100.0) * 1.8;
    $high  = max($open, $close) + mt_rand(0, 80) / 100.0;
    $low   = min($open, $close) - mt_rand(0, 80) / 100.0;
    $vol   = 50000 + mt_rand(0, 30000);
    $rows[] = [$base + $i * 86400, $open, $high, $low, $close, $vol];
    $price = $close;
}
(new FastChart\StockChart(800, 460))
    ->setTitle('ACME 90 day OHLCV')
    ->setOhlcv($rows)
    ->addMovingAverage(10, FastChart\StockChart::MA_SMA)
    ->addMovingAverage(20, FastChart\StockChart::MA_EMA)
    ->addMovingAverage(30, FastChart\StockChart::MA_WMA)
    ->setVolumePane(true)
    ->setCandleStyle(FastChart\Chart::STYLE_CANDLE)
    ->setLegendPosition(FastChart\Chart::LEGEND_BOTTOM_LEFT);
PHP,
    'build' => function () use ($font, $dpi) {
        mt_srand(1);
        $rows = [];
        $base = 1700000000;
        $price = 100.0;
        for ($i = 0; $i < 90; $i++) {
            $open  = $price;
            $close = $price + (mt_rand(-100, 100) / 100.0) * 1.8;
            $high  = max($open, $close) + mt_rand(0, 80) / 100.0;
            $low   = min($open, $close) - mt_rand(0, 80) / 100.0;
            $vol   = 50000 + mt_rand(0, 30000);
            $rows[] = [$base + $i * 86400, $open, $high, $low, $close, $vol];
            $price = $close;
        }
        return (new FastChart\StockChart(800, 460))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('ACME 90 day OHLCV')
            ->setOhlcv($rows)
            ->addMovingAverage(10, FastChart\StockChart::MA_SMA)
            ->addMovingAverage(20, FastChart\StockChart::MA_EMA)
            ->addMovingAverage(30, FastChart\StockChart::MA_WMA)
            ->setVolumePane(true)
            ->setCandleStyle(FastChart\Chart::STYLE_CANDLE)
            ->setLegendPosition(FastChart\Chart::LEGEND_BOTTOM_LEFT);
    },
];

$cases[] = [
    'label' => '7. RadarChart — feature parity scorecard',
    'ref'   => 'docs/examples/08_radar.php',
    'code'  => <<<'PHP'
(new FastChart\RadarChart(560, 480))
    ->setTitle('Feature parity scorecard')
    ->setSeries([
        ['label' => 'Product A', 'data' => [8, 7, 9, 6, 8, 7]],
        ['label' => 'Product B', 'data' => [6, 9, 7, 8, 5, 9]],
    ])
    ->setCategoryLabels(['Speed','Reliability','API','Docs','Support','Pricing'])
    ->setMaxValue(10)
    ->setSeriesColors([0x4F86C6, 0xE34A6F]);
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\RadarChart(560, 480))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Feature parity scorecard')
            ->setSeries([
                ['label' => 'Product A', 'data' => [8, 7, 9, 6, 8, 7]],
                ['label' => 'Product B', 'data' => [6, 9, 7, 8, 5, 9]],
            ])
            ->setCategoryLabels(['Speed', 'Reliability', 'API', 'Docs', 'Support', 'Pricing'])
            ->setMaxValue(10)
            ->setSeriesColors([0x4F86C6, 0xE34A6F]);
    },
];

$cases[] = [
    'label' => '8. PolarChart — antenna gain pattern',
    'ref'   => 'docs/examples/16_polar.php',
    'code'  => <<<'PHP'
$pts_a = []; $pts_b = [];
for ($a = 0; $a <= 360; $a += 15) {
    $pts_a[] = [$a, 4 + 3 * sin(deg2rad($a) * 2)];
    $pts_b[] = [$a, 3 + 2 * cos(deg2rad($a))];
}
(new FastChart\PolarChart(520, 520))
    ->setTitle('Antenna gain pattern')
    ->setSeries([
        ['label' => 'Antenna A', 'data' => $pts_a],
        ['label' => 'Antenna B', 'data' => $pts_b],
    ])
    ->setMaxRadius(8)
    ->setSeriesColors([0x4F86C6, 0xE34A6F]);
PHP,
    'build' => function () use ($font, $dpi) {
        $pts_a = []; $pts_b = [];
        for ($a = 0; $a <= 360; $a += 15) {
            $pts_a[] = [$a, 4 + 3 * sin(deg2rad($a) * 2)];
            $pts_b[] = [$a, 3 + 2 * cos(deg2rad($a))];
        }
        return (new FastChart\PolarChart(520, 520))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Antenna gain pattern')
            ->setSeries([
                ['label' => 'Antenna A', 'data' => $pts_a],
                ['label' => 'Antenna B', 'data' => $pts_b],
            ])
            ->setMaxRadius(8)
            ->setSeriesColors([0x4F86C6, 0xE34A6F]);
    },
];

$cases[] = [
    'label' => '9. SurfaceChart — 2D scalar field',
    'ref'   => 'docs/examples/15_surface_contour.php',
    'code'  => <<<'PHP'
$grid = [];
for ($r = 0; $r < 16; $r++) {
    $row = [];
    for ($c = 0; $c < 24; $c++) {
        $row[] = sin($c / 4.0) * cos($r / 3.0) * 10.0;
    }
    $grid[] = $row;
}
(new FastChart\SurfaceChart(560, 320))
    ->setTitle('Surface heatmap')
    ->setGrid($grid)
    ->setColorRamp(0x1F4E79, 0xE34A6F);
PHP,
    'build' => function () use ($font, $dpi) {
        $grid = [];
        for ($r = 0; $r < 16; $r++) {
            $row = [];
            for ($c = 0; $c < 24; $c++) {
                $row[] = sin($c / 4.0) * cos($r / 3.0) * 10.0;
            }
            $grid[] = $row;
        }
        return (new FastChart\SurfaceChart(560, 320))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Surface heatmap')
            ->setGrid($grid)
            ->setColorRamp(0x1F4E79, 0xE34A6F)
            ->setShowCellValues(false);
    },
];

$cases[] = [
    'label' => '10. ContourChart — filled isolines',
    'ref'   => 'docs/examples/15_surface_contour.php',
    'code'  => <<<'PHP'
$grid = [];
for ($r = 0; $r < 16; $r++) {
    $row = [];
    for ($c = 0; $c < 24; $c++) {
        $row[] = sin($c / 4.0) * cos($r / 3.0) * 10.0;
    }
    $grid[] = $row;
}
(new FastChart\ContourChart(560, 320))
    ->setTitle('Filled contour at 7 levels')
    ->setGrid($grid)
    ->setLevels([-8, -4, -2, 0, 2, 4, 8])
    ->setFilled(true)
    ->setColorRamp(0x1F4E79, 0xE34A6F);
PHP,
    'build' => function () use ($font, $dpi) {
        $grid = [];
        for ($r = 0; $r < 16; $r++) {
            $row = [];
            for ($c = 0; $c < 24; $c++) {
                $row[] = sin($c / 4.0) * cos($r / 3.0) * 10.0;
            }
            $grid[] = $row;
        }
        return (new FastChart\ContourChart(560, 320))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Filled contour at 7 levels')
            ->setGrid($grid)
            ->setLevels([-8, -4, -2, 0, 2, 4, 8])
            ->setFilled(true)
            ->setColorRamp(0x1F4E79, 0xE34A6F);
    },
];

$cases[] = [
    'label' => '11. PieChart — donut with exploded slice',
    'ref'   => 'docs/examples/05_pie_donut.php',
    'code'  => <<<'PHP'
(new FastChart\PieChart(560, 400))
    ->setTitle('Traffic source breakdown')
    ->setSlices([
        'Organic search' => 41, 'Direct'   => 28,
        'Referral'       => 15, 'Social'   =>  9,
        'Email'          =>  5, 'Paid'     =>  2,
    ])
    ->setDonutHoleRatio(0.5)
    ->setExplode([0 => 12])
    ->setSliceLabelPosition(FastChart\Chart::LABEL_OUTSIDE)
    ->setSliceLabelFormat('%.0f%%')
    ->setSeriesColors([0x4F86C6, 0xF6AE2D, 0xE34A6F, 0x4FB286, 0x9B5DE5, 0x9D9D9D]);
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\PieChart(560, 400))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Traffic source breakdown')
            ->setSlices([
                'Organic search' => 41,
                'Direct'         => 28,
                'Referral'       => 15,
                'Social'         =>  9,
                'Email'          =>  5,
                'Paid'           =>  2,
            ])
            ->setDonutHoleRatio(0.5)
            ->setExplode([0 => 12])
            ->setSliceLabelPosition(FastChart\Chart::LABEL_OUTSIDE)
            ->setSliceLabelFormat('%.0f%%')
            ->setSeriesColors([0x4F86C6, 0xF6AE2D, 0xE34A6F, 0x4FB286, 0x9B5DE5, 0x9D9D9D]);
    },
];

$cases[] = [
    'label' => '12. GaugeChart — zoned half-circle',
    'ref'   => 'docs/examples/10_gauge.php',
    'code'  => <<<'PHP'
(new FastChart\GaugeChart(480, 320))
    ->setTitle('Server CPU utilization')
    ->setRange(0, 100)
    ->setValue(72)
    ->setZones([
        ['from' => 0,  'to' => 50,  'color' => 0x4FB286],
        ['from' => 50, 'to' => 80,  'color' => 0xF6AE2D],
        ['from' => 80, 'to' => 100, 'color' => 0xE34A6F],
    ])
    ->setValueFormat('%.0f%%');
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\GaugeChart(480, 320))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Server CPU utilization')
            ->setRange(0, 100)
            ->setValue(72)
            ->setZones([
                ['from' => 0,  'to' => 50,  'color' => 0x4FB286],
                ['from' => 50, 'to' => 80,  'color' => 0xF6AE2D],
                ['from' => 80, 'to' => 100, 'color' => 0xE34A6F],
            ])
            ->setValueFormat('%.0f%%');
    },
];

$cases[] = [
    'label' => '13. LinearMeter — horizontal SLA bar',
    'ref'   => 'docs/examples/36_linear_meter.php',
    'code'  => <<<'PHP'
(new FastChart\LinearMeter(680, 200))
    ->setTitle('CPU utilisation')
    ->setRange(0, 100)
    ->setValue(72)
    ->setZones([
        ['from' => 0,  'to' => 60,  'color' => 0x4FB286],
        ['from' => 60, 'to' => 85,  'color' => 0xE8A33D],
        ['from' => 85, 'to' => 100, 'color' => 0xE34A6F],
    ])
    ->setValueFormat('%.0f%%');
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\LinearMeter(680, 200))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('CPU utilisation')
            ->setRange(0, 100)
            ->setValue(72)
            ->setZones([
                ['from' => 0,  'to' => 60,  'color' => 0x4FB286],
                ['from' => 60, 'to' => 85,  'color' => 0xE8A33D],
                ['from' => 85, 'to' => 100, 'color' => 0xE34A6F],
            ])
            ->setValueFormat('%.0f%%');
    },
];

$cases[] = [
    'label' => '14. GanttChart — Q4 release timeline',
    'ref'   => 'docs/examples/17_gantt.php',
    'code'  => <<<'PHP'
$base = 1700000000; $day = 86400;
(new FastChart\GanttChart(720, 380))
    ->setTitle('Q4 release timeline')
    ->setTimeRange($base, $base + 60 * $day)
    ->setTasks([
        ['name' => 'Spec',   'start' => $base + 0 * $day,  'end' => $base + 7  * $day, 'color' => 0x4F86C6],
        ['name' => 'Design', 'start' => $base + 5 * $day,  'end' => $base + 18 * $day, 'depends' => [0]],
        ['name' => 'Build',  'start' => $base + 15 * $day, 'end' => $base + 38 * $day, 'depends' => [1]],
        ['name' => 'QA',     'start' => $base + 35 * $day, 'end' => $base + 50 * $day, 'depends' => [2]],
        ['name' => 'Launch', 'start' => $base + 50 * $day, 'end' => $base + 50 * $day,
         'milestone' => true, 'color' => 0xE34A6F],
    ])
    ->setShowTaskLabels(true);
PHP,
    'build' => function () use ($font, $dpi) {
        $base = 1700000000;
        $day = 86400;
        return (new FastChart\GanttChart(720, 380))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Q4 release timeline')
            ->setTimeRange($base, $base + 60 * $day)
            ->setTasks([
                ['name' => 'Spec',   'start' => $base + 0 * $day,  'end' => $base + 7  * $day, 'color' => 0x4F86C6],
                ['name' => 'Design', 'start' => $base + 5 * $day,  'end' => $base + 18 * $day, 'depends' => [0]],
                ['name' => 'Build',  'start' => $base + 15 * $day, 'end' => $base + 38 * $day, 'depends' => [1]],
                ['name' => 'QA',     'start' => $base + 35 * $day, 'end' => $base + 50 * $day, 'depends' => [2]],
                ['name' => 'Launch', 'start' => $base + 50 * $day, 'end' => $base + 50 * $day, 'milestone' => true, 'color' => 0xE34A6F],
            ])
            ->setShowTaskLabels(true);
    },
];

$cases[] = [
    'label' => '15. BoxPlot — latency by service',
    'ref'   => 'docs/examples/09_boxplot.php',
    'code'  => <<<'PHP'
(new FastChart\BoxPlot(640, 400))
    ->setTitle('Response time distribution by service')
    ->setYAxisTitle('Latency (ms)')
    ->setBoxes([
        ['label' => 'auth',    'min' => 18, 'q1' =>  32, 'median' =>  45, 'q3' => 58,  'max' =>  95, 'outliers' => [120, 135]],
        ['label' => 'profile', 'min' => 42, 'q1' =>  65, 'median' =>  82, 'q3' => 105, 'max' => 160, 'outliers' => [220]],
        ['label' => 'search',  'min' => 80, 'q1' => 110, 'median' => 145, 'q3' => 195, 'max' => 320, 'outliers' => [410, 480, 510]],
        ['label' => 'orders',  'min' => 25, 'q1' =>  38, 'median' =>  52, 'q3' =>  71, 'max' => 105],
    ]);
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\BoxPlot(640, 400))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Response time distribution by service')
            ->setYAxisTitle('Latency (ms)')
            ->setBoxes([
                ['label' => 'auth',    'min' => 18, 'q1' =>  32, 'median' =>  45, 'q3' => 58,  'max' =>  95, 'outliers' => [120, 135]],
                ['label' => 'profile', 'min' => 42, 'q1' =>  65, 'median' =>  82, 'q3' => 105, 'max' => 160, 'outliers' => [220]],
                ['label' => 'search',  'min' => 80, 'q1' => 110, 'median' => 145, 'q3' => 195, 'max' => 320, 'outliers' => [410, 480, 510]],
                ['label' => 'orders',  'min' => 25, 'q1' =>  38, 'median' =>  52, 'q3' =>  71, 'max' => 105],
            ]);
    },
];

$cases[] = [
    'label' => '16. Treemap — revenue by product line',
    'ref'   => 'docs/examples/32_treemap.php',
    'code'  => <<<'PHP'
(new FastChart\Treemap(720, 440))
    ->setTitle('Q3 revenue by product line')
    ->setItems([
        ['label' => 'Cloud',     'value' => 4200],
        ['label' => 'Hardware',  'value' => 2900],
        ['label' => 'Services',  'value' => 2100],
        ['label' => 'Licenses',  'value' => 1500],
        ['label' => 'Training',  'value' =>  900],
        ['label' => 'Support',   'value' =>  700],
        ['label' => 'Marketing', 'value' =>  400, 'color' => 0xE34A6F],
        ['label' => 'Other',     'value' =>  300, 'color' => 0x9B9B9B],
    ]);
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\Treemap(720, 440))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Q3 revenue by product line')
            ->setItems([
                ['label' => 'Cloud',     'value' => 4200],
                ['label' => 'Hardware',  'value' => 2900],
                ['label' => 'Services',  'value' => 2100],
                ['label' => 'Licenses',  'value' => 1500],
                ['label' => 'Training',  'value' =>  900],
                ['label' => 'Support',   'value' =>  700],
                ['label' => 'Marketing', 'value' =>  400, 'color' => 0xE34A6F],
                ['label' => 'Other',     'value' =>  300, 'color' => 0x9B9B9B],
            ]);
    },
];

$cases[] = [
    'label' => '17. Funnel — signup → paid conversion',
    'ref'   => 'docs/examples/33_funnel.php',
    'code'  => <<<'PHP'
(new FastChart\Funnel(680, 420))
    ->setTitle('Signup → paid conversion')
    ->setStages([
        ['label' => 'Visit',   'value' => 12_400],
        ['label' => 'Sign up', 'value' =>  3_200],
        ['label' => 'Active',  'value' =>  1_950],
        ['label' => 'Trial',   'value' =>    820],
        ['label' => 'Paid',    'value' =>    310],
    ]);
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\Funnel(680, 420))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Signup → paid conversion')
            ->setStages([
                ['label' => 'Visit',   'value' => 12_400],
                ['label' => 'Sign up', 'value' =>  3_200],
                ['label' => 'Active',  'value' =>  1_950],
                ['label' => 'Trial',   'value' =>    820],
                ['label' => 'Paid',    'value' =>    310],
            ]);
    },
];

$cases[] = [
    'label' => '18. Waterfall — Q3 income statement',
    'ref'   => 'docs/examples/34_waterfall.php',
    'code'  => <<<'PHP'
(new FastChart\Waterfall(720, 400))
    ->setTitle('Q3 income statement ($M)')
    ->setBars([
        ['label' => 'Revenue', 'value' => 1200, 'kind' => 'total'],
        ['label' => 'COGS',    'value' => -480],
        ['label' => 'Margin',  'value' =>  720, 'kind' => 'total'],
        ['label' => 'R&D',     'value' => -180],
        ['label' => 'S&M',     'value' => -220],
        ['label' => 'G&A',     'value' =>  -90],
        ['label' => 'OpInc',   'value' =>  230, 'kind' => 'total'],
    ]);
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\Waterfall(720, 400))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Q3 income statement ($M)')
            ->setBars([
                ['label' => 'Revenue', 'value' => 1200, 'kind' => 'total'],
                ['label' => 'COGS',    'value' => -480],
                ['label' => 'Margin',  'value' =>  720, 'kind' => 'total'],
                ['label' => 'R&D',     'value' => -180],
                ['label' => 'S&M',     'value' => -220],
                ['label' => 'G&A',     'value' =>  -90],
                ['label' => 'OpInc',   'value' =>  230, 'kind' => 'total'],
            ]);
    },
];

$cases[] = [
    'label' => '19. Heatmap — hourly traffic, last week',
    'ref'   => 'docs/examples/35_heatmap.php',
    'code'  => <<<'PHP'
$grid = [];
for ($d = 0; $d < 7; $d++) {
    $row = [];
    for ($h = 0; $h < 24; $h++) {
        $weekday = $d < 5 ? 1 : 0;
        $morning = exp(-pow($h - 9, 2) / 12);
        $evening = exp(-pow($h - 21, 2) / 8);
        $row[] = 20 + 80 * $weekday * $morning + 60 * (1 - $weekday) * $evening
              + sin($d + $h) * 4;
    }
    $grid[] = $row;
}
(new FastChart\Heatmap(720, 280))
    ->setTitle('Hourly traffic, last week')
    ->setGrid($grid)
    ->setColorRamp(0xF1F5F9, 0xE34A6F);
PHP,
    'build' => function () use ($font, $dpi) {
        $grid = [];
        for ($d = 0; $d < 7; $d++) {
            $row = [];
            for ($h = 0; $h < 24; $h++) {
                $weekday = $d < 5 ? 1 : 0;
                $morning = exp(-pow($h - 9, 2) / 12);
                $evening = exp(-pow($h - 21, 2) / 8);
                $row[] = 20
                      + 80 * $weekday * $morning
                      + 60 * (1 - $weekday) * $evening
                      + sin($d + $h) * 4;
            }
            $grid[] = $row;
        }
        return (new FastChart\Heatmap(720, 280))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Hourly traffic, last week')
            ->setGrid($grid)
            ->setColorRamp(0xF1F5F9, 0xE34A6F);
    },
];

$cases[] = [
    'label' => '20. BulletChart — Q3 revenue vs plan',
    'ref'   => 'docs/examples/43_bullet.php',
    'code'  => <<<'PHP'
(new FastChart\BulletChart(560, 120))
    ->setTitle('Q3 revenue vs plan')
    ->setRange(0, 120)
    ->setBands([
        ['from' =>   0, 'to' =>  60, 'color' => 0xE5E7EB],
        ['from' =>  60, 'to' =>  90, 'color' => 0xCBD5E1],
        ['from' =>  90, 'to' => 120, 'color' => 0x94A3B8],
    ])
    ->setValue(78)
    ->setTarget(95)
    ->setValueFormat('%.0fM');
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\BulletChart(560, 120))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Q3 revenue vs plan')
            ->setRange(0, 120)
            ->setBands([
                ['from' =>   0, 'to' =>  60, 'color' => 0xE5E7EB],
                ['from' =>  60, 'to' =>  90, 'color' => 0xCBD5E1],
                ['from' =>  90, 'to' => 120, 'color' => 0x94A3B8],
            ])
            ->setValue(78)
            ->setTarget(95)
            ->setValueFormat('%.0fM');
    },
];

$cases[] = [
    'label' => '21. ParetoChart — defect categories with 80/20 line',
    'ref'   => 'docs/examples/44_pareto.php',
    'code'  => <<<'PHP'
(new FastChart\ParetoChart(680, 420))
    ->setTitle('Defect categories, last quarter')
    ->setBars([
        ['label' => 'Soldering', 'value' => 142],
        ['label' => 'Plating',   'value' =>  88],
        ['label' => 'Etching',   'value' =>  61],
        ['label' => 'Drilling',  'value' =>  34],
        ['label' => 'Cutting',   'value' =>  21],
        ['label' => 'Other',     'value' =>  12],
    ])
    ->setLineColor(0xE34A6F)
    ->setShowValues(true);
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\ParetoChart(680, 420))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Defect categories, last quarter')
            ->setBars([
                ['label' => 'Soldering', 'value' => 142],
                ['label' => 'Plating',   'value' =>  88],
                ['label' => 'Etching',   'value' =>  61],
                ['label' => 'Drilling',  'value' =>  34],
                ['label' => 'Cutting',   'value' =>  21],
                ['label' => 'Other',     'value' =>  12],
            ])
            ->setLineColor(0xE34A6F)
            ->setShowValues(true);
    },
];

$cases[] = [
    'label' => '22. CalendarHeatmap — daily activity for one year',
    'ref'   => 'docs/examples/45_calendar_heatmap.php',
    'code'  => <<<'PHP'
$data = [];
$start = strtotime('2025-05-01');
$end   = strtotime('2026-04-30');
for ($ts = $start; $ts <= $end; $ts += 86400) {
    $iso = date('Y-m-d', $ts);
    $dow = (int)date('w', $ts);
    $base = $dow >= 1 && $dow <= 5 ? 6 : 1;
    $burst = sin($ts / 86400 / 7) * 4 + cos($ts / 86400 / 3) * 3;
    $v = max(0, $base + $burst);
    if ($v > 0) $data[$iso] = round($v);
}
(new FastChart\CalendarHeatmap(900, 170))
    ->setTitle('Daily commits, last 12 months')
    ->setData($data)
    ->setColorRamp(0xEBEDEF, 0x216E39);
PHP,
    'build' => function () use ($font, $dpi) {
        $data = [];
        $start = strtotime('2025-05-01');
        $end   = strtotime('2026-04-30');
        for ($ts = $start; $ts <= $end; $ts += 86400) {
            $iso = date('Y-m-d', $ts);
            $dow = (int)date('w', $ts);
            $base = $dow >= 1 && $dow <= 5 ? 6 : 1;
            $burst = sin($ts / 86400 / 7) * 4 + cos($ts / 86400 / 3) * 3;
            $v = max(0, $base + $burst);
            if ($v > 0) $data[$iso] = round($v);
        }
        return (new FastChart\CalendarHeatmap(900, 170))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Daily commits, last 12 months')
            ->setData($data)
            ->setColorRamp(0xEBEDEF, 0x216E39);
    },
];

$cases[] = [
    'label' => '23. SunburstChart — engineering workload by team',
    'ref'   => 'docs/examples/46_sunburst.php',
    'code'  => <<<'PHP'
(new FastChart\SunburstChart(520, 520))
    ->setTitle('Workload by team & project')
    ->setHierarchy([
        'label' => 'Eng',
        'children' => [
            ['label' => 'Backend', 'children' => [
                ['label' => 'API',     'value' => 18],
                ['label' => 'Workers', 'value' => 12],
                ['label' => 'DB',      'value' =>  9],
            ]],
            ['label' => 'Frontend', 'children' => [
                ['label' => 'Web',    'value' => 14],
                ['label' => 'Mobile', 'value' => 10],
            ]],
            ['label' => 'Infra', 'children' => [
                ['label' => 'CI',       'value' => 7],
                ['label' => 'Observ.',  'value' => 5],
                ['label' => 'Security', 'value' => 4],
            ]],
        ],
    ]);
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\SunburstChart(520, 520))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Workload by team & project')
            ->setHierarchy([
                'label' => 'Eng',
                'children' => [
                    ['label' => 'Backend', 'children' => [
                        ['label' => 'API',     'value' => 18],
                        ['label' => 'Workers', 'value' => 12],
                        ['label' => 'DB',      'value' =>  9],
                    ]],
                    ['label' => 'Frontend', 'children' => [
                        ['label' => 'Web',    'value' => 14],
                        ['label' => 'Mobile', 'value' => 10],
                    ]],
                    ['label' => 'Infra', 'children' => [
                        ['label' => 'CI',       'value' => 7],
                        ['label' => 'Observ.',  'value' => 5],
                        ['label' => 'Security', 'value' => 4],
                    ]],
                ],
            ]);
    },
];

$cases[] = [
    'label' => '24. SankeyChart — store orders (4-layer: store → category → item → brand)',
    'ref'   => 'docs/examples/47_sankey.php',
    'code'  => <<<'PHP'
/* Four-layer e-commerce flow with one many-to-one path:
 * Mobile + Tablet both feed Samsung. The rest are 1:1. */
(new FastChart\SankeyChart(860, 540))
    ->setTitle('Store orders: category -> item -> brand')
    ->setNodes([
        /* 0 */  ['label' => 'Online Store (621)', 'color' => 0x6C8EBF],
        /* 1 */  ['label' => 'Furnitures (162)',   'color' => 0xD6B656],
        /* 2 */  ['label' => 'Garments (191)',     'color' => 0x6FD6D6],
        /* 3 */  ['label' => 'Electronics (268)',  'color' => 0xC993D6],
        /* 4 */  ['label' => 'Desk (43)'],
        /* 5 */  ['label' => 'Sofa (73)'],
        /* 6 */  ['label' => 'Chair (46)'],
        /* 7 */  ['label' => 'Jeans (46)'],
        /* 8 */  ['label' => 'T-Shirt (104)'],
        /* 9 */  ['label' => 'Jackets (41)'],
        /* 10 */ ['label' => 'Mobile (39)'],
        /* 11 */ ['label' => 'Tablet (73)'],
        /* 12 */ ['label' => 'Laptop (156)'],
        /* 13 */ ['label' => 'Stickley (43)'],
        /* 14 */ ['label' => 'IKEA (73)'],
        /* 15 */ ['label' => 'Kartell (46)'],
        /* 16 */ ['label' => "Levi's (46)"],
        /* 17 */ ['label' => 'H&M (104)'],
        /* 18 */ ['label' => 'Puma (41)'],
        /* 19 */ ['label' => 'Samsung (112)'],
        /* 20 */ ['label' => 'Dell (156)'],
    ])
    ->setLinks([
        ['from' => 0, 'to' => 1, 'value' => 162],
        ['from' => 0, 'to' => 2, 'value' => 191],
        ['from' => 0, 'to' => 3, 'value' => 268],
        ['from' => 1, 'to' => 4, 'value' =>  43],
        ['from' => 1, 'to' => 5, 'value' =>  73],
        ['from' => 1, 'to' => 6, 'value' =>  46],
        ['from' => 2, 'to' => 7, 'value' =>  46],
        ['from' => 2, 'to' => 8, 'value' => 104],
        ['from' => 2, 'to' => 9, 'value' =>  41],
        ['from' => 3, 'to' => 10, 'value' =>  39],
        ['from' => 3, 'to' => 11, 'value' =>  73],
        ['from' => 3, 'to' => 12, 'value' => 156],
        ['from' =>  4, 'to' => 13, 'value' =>  43],
        ['from' =>  5, 'to' => 14, 'value' =>  73],
        ['from' =>  6, 'to' => 15, 'value' =>  46],
        ['from' =>  7, 'to' => 16, 'value' =>  46],
        ['from' =>  8, 'to' => 17, 'value' => 104],
        ['from' =>  9, 'to' => 18, 'value' =>  41],
        ['from' => 10, 'to' => 19, 'value' =>  39],
        ['from' => 11, 'to' => 19, 'value' =>  73],
        ['from' => 12, 'to' => 20, 'value' => 156],
    ]);
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\SankeyChart(860, 540))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Store orders: category -> item -> brand')
            ->setNodes([
                ['label' => 'Online Store (621)', 'color' => 0x6C8EBF],
                ['label' => 'Furnitures (162)',   'color' => 0xD6B656],
                ['label' => 'Garments (191)',     'color' => 0x6FD6D6],
                ['label' => 'Electronics (268)',  'color' => 0xC993D6],
                ['label' => 'Desk (43)'],
                ['label' => 'Sofa (73)'],
                ['label' => 'Chair (46)'],
                ['label' => 'Jeans (46)'],
                ['label' => 'T-Shirt (104)'],
                ['label' => 'Jackets (41)'],
                ['label' => 'Mobile (39)'],
                ['label' => 'Tablet (73)'],
                ['label' => 'Laptop (156)'],
                ['label' => 'Stickley (43)'],
                ['label' => 'IKEA (73)'],
                ['label' => 'Kartell (46)'],
                ['label' => "Levi's (46)"],
                ['label' => 'H&M (104)'],
                ['label' => 'Puma (41)'],
                ['label' => 'Samsung (112)'],
                ['label' => 'Dell (156)'],
            ])
            ->setLinks([
                ['from' => 0, 'to' => 1, 'value' => 162],
                ['from' => 0, 'to' => 2, 'value' => 191],
                ['from' => 0, 'to' => 3, 'value' => 268],
                ['from' => 1, 'to' => 4, 'value' =>  43],
                ['from' => 1, 'to' => 5, 'value' =>  73],
                ['from' => 1, 'to' => 6, 'value' =>  46],
                ['from' => 2, 'to' => 7, 'value' =>  46],
                ['from' => 2, 'to' => 8, 'value' => 104],
                ['from' => 2, 'to' => 9, 'value' =>  41],
                ['from' => 3, 'to' => 10, 'value' =>  39],
                ['from' => 3, 'to' => 11, 'value' =>  73],
                ['from' => 3, 'to' => 12, 'value' => 156],
                ['from' =>  4, 'to' => 13, 'value' =>  43],
                ['from' =>  5, 'to' => 14, 'value' =>  73],
                ['from' =>  6, 'to' => 15, 'value' =>  46],
                ['from' =>  7, 'to' => 16, 'value' =>  46],
                ['from' =>  8, 'to' => 17, 'value' => 104],
                ['from' =>  9, 'to' => 18, 'value' =>  41],
                ['from' => 10, 'to' => 19, 'value' =>  39],
                ['from' => 11, 'to' => 19, 'value' =>  73],
                ['from' => 12, 'to' => 20, 'value' => 156],
            ]);
    },
];

$cases[] = [
    'label' => '25. MarimekkoChart — revenue mix by region & product',
    'ref'   => 'docs/examples/48_marimekko.php',
    'code'  => <<<'PHP'
(new FastChart\MarimekkoChart(700, 480))
    ->setTitle('Revenue mix by region & product')
    ->setColumns([
        ['label' => 'NA',   'segments' => [
            ['label' => 'Hardware', 'value' => 80, 'color' => 0x2563EB],
            ['label' => 'Services', 'value' => 50, 'color' => 0x10B981],
            ['label' => 'Cloud',    'value' => 70, 'color' => 0xF59E0B],
        ]],
        ['label' => 'EMEA', 'segments' => [
            ['label' => 'Hardware', 'value' => 45, 'color' => 0x2563EB],
            ['label' => 'Services', 'value' => 35, 'color' => 0x10B981],
            ['label' => 'Cloud',    'value' => 25, 'color' => 0xF59E0B],
        ]],
        ['label' => 'APAC', 'segments' => [
            ['label' => 'Hardware', 'value' => 30, 'color' => 0x2563EB],
            ['label' => 'Services', 'value' => 15, 'color' => 0x10B981],
            ['label' => 'Cloud',    'value' => 40, 'color' => 0xF59E0B],
        ]],
        ['label' => 'LATAM','segments' => [
            ['label' => 'Hardware', 'value' => 12, 'color' => 0x2563EB],
            ['label' => 'Services', 'value' =>  8, 'color' => 0x10B981],
        ]],
    ]);
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\MarimekkoChart(700, 480))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Revenue mix by region & product')
            ->setColumns([
                ['label' => 'NA',   'segments' => [
                    ['label' => 'Hardware', 'value' => 80, 'color' => 0x2563EB],
                    ['label' => 'Services', 'value' => 50, 'color' => 0x10B981],
                    ['label' => 'Cloud',    'value' => 70, 'color' => 0xF59E0B],
                ]],
                ['label' => 'EMEA', 'segments' => [
                    ['label' => 'Hardware', 'value' => 45, 'color' => 0x2563EB],
                    ['label' => 'Services', 'value' => 35, 'color' => 0x10B981],
                    ['label' => 'Cloud',    'value' => 25, 'color' => 0xF59E0B],
                ]],
                ['label' => 'APAC', 'segments' => [
                    ['label' => 'Hardware', 'value' => 30, 'color' => 0x2563EB],
                    ['label' => 'Services', 'value' => 15, 'color' => 0x10B981],
                    ['label' => 'Cloud',    'value' => 40, 'color' => 0xF59E0B],
                ]],
                ['label' => 'LATAM','segments' => [
                    ['label' => 'Hardware', 'value' => 12, 'color' => 0x2563EB],
                    ['label' => 'Services', 'value' =>  8, 'color' => 0x10B981],
                ]],
            ]);
    },
];

$cases[] = [
    'label' => '26. VectorChart — rotational vector field',
    'ref'   => 'docs/examples/49_vector.php',
    'code'  => <<<'PHP'
$vecs = [];
for ($x = 0; $x <= 10; $x++) {
    for ($y = 0; $y <= 10; $y++) {
        $vecs[] = ['x' => $x, 'y' => $y,
                   'dx' => -($y - 5) * 0.3,
                   'dy' =>  ($x - 5) * 0.3];
    }
}
(new FastChart\VectorChart(560, 520))
    ->setTitle('Rotational vector field')
    ->setVectors($vecs)
    ->setColorRamp(0xDDE7FF, 0x1E3A8A);
PHP,
    'build' => function () use ($font, $dpi) {
        $vecs = [];
        for ($x = 0; $x <= 10; $x++) {
            for ($y = 0; $y <= 10; $y++) {
                $vecs[] = ['x' => $x, 'y' => $y,
                           'dx' => -($y - 5) * 0.3,
                           'dy' =>  ($x - 5) * 0.3];
            }
        }
        return (new FastChart\VectorChart(560, 520))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Rotational vector field')
            ->setVectors($vecs)
            ->setColorRamp(0xDDE7FF, 0x1E3A8A);
    },
];

$cases[] = [
    'label' => '27a. Code128 — alphanumeric, human-readable text',
    'ref'   => 'docs/examples/41_code128.php',
    'code'  => <<<'PHP'
(new FastChart\Code128())
    ->setData('FASTCHART-12345')
    ->setSize(400, 100)
    ->setShowText(true);
PHP,
    'build' => function () use ($dpi) {
        return (new FastChart\Code128())
            ->setData('FASTCHART-12345')
            ->setSize(400, 100)
            ->setDpi($dpi)
            ->setShowText(true);
    },
];

$cases[] = [
    'label' => '27b. Code128 — pure digits (encoded entirely in subset C)',
    'ref'   => 'docs/examples/41_code128.php',
    'code'  => <<<'PHP'
(new FastChart\Code128())
    ->setData('0123456789012345')
    ->setSize(360, 80)
    ->setShowText(true);
PHP,
    'build' => function () use ($dpi) {
        return (new FastChart\Code128())
            ->setData('0123456789012345')
            ->setSize(360, 80)
            ->setDpi($dpi)
            ->setShowText(true);
    },
];

$cases[] = [
    'label' => '27c. Code128 — compact bars only, no human-readable text',
    'ref'   => 'docs/examples/41_code128.php',
    'code'  => <<<'PHP'
(new FastChart\Code128())
    ->setData('FASTCHART-12345')
    ->setSize(400, 60)
    ->setShowText(false);
PHP,
    'build' => function () use ($dpi) {
        return (new FastChart\Code128())
            ->setData('FASTCHART-12345')
            ->setSize(400, 60)
            ->setDpi($dpi)
            ->setShowText(false);
    },
];

$cases[] = [
    'label' => '28a. QrCode — ECC level L (~7% recovery, smallest symbol)',
    'ref'   => 'docs/examples/42_qrcode.php',
    'code'  => <<<'PHP'
(new FastChart\QrCode())
    ->setData('https://github.com/iliaal/fastchart')
    ->setSize(300, 300)
    ->setEcc(FastChart\QrCode::ECC_L)
    ->setQuietZone(4);
PHP,
    'build' => function () use ($dpi) {
        return (new FastChart\QrCode())
            ->setData('https://github.com/iliaal/fastchart')
            ->setSize(300, 300)
            ->setDpi($dpi)
            ->setEcc(FastChart\QrCode::ECC_L)
            ->setQuietZone(4);
    },
];

$cases[] = [
    'label' => '28b. QrCode — ECC level M (~15% recovery, default)',
    'ref'   => 'docs/examples/42_qrcode.php',
    'code'  => <<<'PHP'
(new FastChart\QrCode())
    ->setData('https://github.com/iliaal/fastchart')
    ->setSize(300, 300)
    ->setEcc(FastChart\QrCode::ECC_M)
    ->setQuietZone(4);
PHP,
    'build' => function () use ($dpi) {
        return (new FastChart\QrCode())
            ->setData('https://github.com/iliaal/fastchart')
            ->setSize(300, 300)
            ->setDpi($dpi)
            ->setEcc(FastChart\QrCode::ECC_M)
            ->setQuietZone(4);
    },
];

$cases[] = [
    'label' => '28c. QrCode — ECC level Q (~25% recovery, logo overlays)',
    'ref'   => 'docs/examples/42_qrcode.php',
    'code'  => <<<'PHP'
(new FastChart\QrCode())
    ->setData('https://github.com/iliaal/fastchart')
    ->setSize(300, 300)
    ->setEcc(FastChart\QrCode::ECC_Q)
    ->setQuietZone(4);
PHP,
    'build' => function () use ($dpi) {
        return (new FastChart\QrCode())
            ->setData('https://github.com/iliaal/fastchart')
            ->setSize(300, 300)
            ->setDpi($dpi)
            ->setEcc(FastChart\QrCode::ECC_Q)
            ->setQuietZone(4);
    },
];

$cases[] = [
    'label' => '28d. QrCode — ECC level H (~30% recovery, outdoor / damaged)',
    'ref'   => 'docs/examples/42_qrcode.php',
    'code'  => <<<'PHP'
(new FastChart\QrCode())
    ->setData('https://github.com/iliaal/fastchart')
    ->setSize(300, 300)
    ->setEcc(FastChart\QrCode::ECC_H)
    ->setQuietZone(4);
PHP,
    'build' => function () use ($dpi) {
        return (new FastChart\QrCode())
            ->setData('https://github.com/iliaal/fastchart')
            ->setSize(300, 300)
            ->setDpi($dpi)
            ->setEcc(FastChart\QrCode::ECC_H)
            ->setQuietZone(4);
    },
];

$cases[] = [
    'label' => '29. Funnel — STYLE_CONE (3D-look band edges)',
    'ref'   => 'docs/examples/52_funnel_cone.php',
    'code'  => <<<'PHP'
(new FastChart\Funnel(560, 360))
    ->setTitle('Signup → paid conversion (cone)')
    ->setStages([
        ['label' => 'Visit',   'value' => 12_400],
        ['label' => 'Sign up', 'value' =>  3_200],
        ['label' => 'Active',  'value' =>  1_950],
        ['label' => 'Trial',   'value' =>    820],
        ['label' => 'Paid',    'value' =>    310],
    ])
    ->setStyle(FastChart\Funnel::STYLE_CONE);
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\Funnel(560, 360))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Signup → paid conversion (cone)')
            ->setStages([
                ['label' => 'Visit',   'value' => 12_400],
                ['label' => 'Sign up', 'value' =>  3_200],
                ['label' => 'Active',  'value' =>  1_950],
                ['label' => 'Trial',   'value' =>    820],
                ['label' => 'Paid',    'value' =>    310],
            ])
            ->setStyle(FastChart\Funnel::STYLE_CONE);
    },
];

$cases[] = [
    'label' => '30. AreaChart — setBandMode (confidence envelope)',
    'ref'   => 'docs/examples/53_area_band.php',
    'code'  => <<<'PHP'
$mean  = []; $upper = []; $lower = [];
for ($i = 0; $i < 30; $i++) {
    $m = 100 + 4 * $i + 8 * sin($i / 3.0);
    $s = 6 + $i * 0.35;
    $mean[]  = $m; $upper[] = $m + $s; $lower[] = $m - $s;
}
(new FastChart\AreaChart(640, 320))
    ->setTitle('30-day forecast with confidence band')
    ->setSeries([
        ['label' => 'upper bound', 'data' => $upper],
        ['label' => 'lower bound', 'data' => $lower],
    ])
    ->setBandMode(true)
    ->setFillOpacity(96);
PHP,
    'build' => function () use ($font, $dpi) {
        $upper = []; $lower = [];
        for ($i = 0; $i < 30; $i++) {
            $m = 100 + 4 * $i + 8 * sin($i / 3.0);
            $s = 6 + $i * 0.35;
            $upper[] = round($m + $s, 1);
            $lower[] = round($m - $s, 1);
        }
        return (new FastChart\AreaChart(640, 320))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('30-day forecast with confidence band')
            ->setSeries([
                ['label' => 'upper bound', 'data' => $upper],
                ['label' => 'lower bound', 'data' => $lower],
            ])
            ->setBandMode(true)
            ->setFillOpacity(96);
    },
];

$cases[] = [
    'label' => '31. PolarChart — INTERP_SMOOTH + addVectors',
    'ref'   => 'docs/examples/54_polar_smooth_vectors.php',
    'code'  => <<<'PHP'
$points = [];
for ($i = 0; $i < 12; $i++) {
    $a = $i * 30;
    $points[] = [$a, 6 + 3 * sin($a * M_PI / 180 * 3)];
}
(new FastChart\PolarChart(480, 480))
    ->setTitle('Smooth polar curve + wind vectors')
    ->setSeries([['label' => 'envelope', 'data' => $points]])
    ->setMaxRadius(12)
    ->setFilled(true)
    ->setInterpolation(FastChart\Chart::INTERP_SMOOTH)
    ->addVectors([
        ['angle' =>   0, 'radius' => 0, 'angle_to' =>   0, 'radius_to' => 4.5],
        ['angle' =>  90, 'radius' => 0, 'angle_to' =>  90, 'radius_to' => 5.8],
        ['angle' => 180, 'radius' => 0, 'angle_to' => 180, 'radius_to' => 6.1],
        ['angle' => 270, 'radius' => 0, 'angle_to' => 270, 'radius_to' => 4.2],
    ]);
PHP,
    'build' => function () use ($font, $dpi) {
        $points = [];
        for ($i = 0; $i < 12; $i++) {
            $a = $i * 30;
            $points[] = [$a, 6 + 3 * sin($a * M_PI / 180 * 3)];
        }
        return (new FastChart\PolarChart(480, 480))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Smooth polar curve + wind vectors')
            ->setSeries([['label' => 'envelope', 'data' => $points]])
            ->setMaxRadius(12)
            ->setFilled(true)
            ->setInterpolation(FastChart\Chart::INTERP_SMOOTH)
            ->addVectors([
                ['angle' =>   0, 'radius' => 0, 'angle_to' =>   0, 'radius_to' => 4.5],
                ['angle' =>  45, 'radius' => 0, 'angle_to' =>  45, 'radius_to' => 3.2],
                ['angle' =>  90, 'radius' => 0, 'angle_to' =>  90, 'radius_to' => 5.8],
                ['angle' => 135, 'radius' => 0, 'angle_to' => 135, 'radius_to' => 2.4],
                ['angle' => 180, 'radius' => 0, 'angle_to' => 180, 'radius_to' => 6.1],
                ['angle' => 225, 'radius' => 0, 'angle_to' => 225, 'radius_to' => 2.9],
                ['angle' => 270, 'radius' => 0, 'angle_to' => 270, 'radius_to' => 4.2],
                ['angle' => 315, 'radius' => 0, 'angle_to' => 315, 'radius_to' => 3.7],
            ]);
    },
];

$cases[] = [
    'label' => '32. BubbleChart — log Y axis',
    'ref'   => 'docs/examples/55_bubble_log_axis.php',
    'code'  => <<<'PHP'
$points = [];
for ($i = 1; $i <= 18; $i++) {
    $points[] = [$i, pow(2.2, $i / 2.0), 8 + $i * 1.3];
}
(new FastChart\BubbleChart(640, 320))
    ->setTitle('Cost vs throughput (log Y)')
    ->setXAxisTitle('Tier')
    ->setYAxisTitle('Monthly cost ($)')
    ->setPoints($points)
    ->setYAxisScale(FastChart\Chart::SCALE_LOG);
PHP,
    'build' => function () use ($font, $dpi) {
        $points = [];
        for ($i = 1; $i <= 18; $i++) {
            $points[] = [$i, pow(2.2, $i / 2.0), 8 + $i * 1.3];
        }
        return (new FastChart\BubbleChart(640, 320))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Cost vs throughput (log Y)')
            ->setXAxisTitle('Tier')
            ->setYAxisTitle('Monthly cost ($)')
            ->setPoints($points)
            ->setYAxisScale(FastChart\Chart::SCALE_LOG);
    },
];

$cases[] = [
    'label' => '33. BarChart — setImageMap (HTML hot-spots)',
    'ref'   => 'docs/examples/56_image_map_bar_pie.php',
    'code'  => <<<'PHP'
$bar = (new FastChart\BarChart(640, 320))
    ->setTitle('Quarterly revenue')
    ->setSeries([['label' => 'revenue ($M)', 'data' => [12.4, 18.1, 21.7, 25.3]]])
    ->setCategoryLabels(['Q1', 'Q2', 'Q3', 'Q4'])
    ->setImageMap([
        ['href' => '/reports/2026q1', 'tooltip' => 'Q1: $12.4M'],
        ['href' => '/reports/2026q2', 'tooltip' => 'Q2: $18.1M'],
        ['href' => '/reports/2026q3', 'tooltip' => 'Q3: $21.7M'],
        ['href' => '/reports/2026q4', 'tooltip' => 'Q4: $25.3M'],
    ]);
$bar->renderPng();
$html_map = $bar->getImageMap('quarterly');
PHP,
    'build' => function () use ($font, $dpi) {
        return (new FastChart\BarChart(640, 320))
            ->setFontPath($font)->setDpi($dpi)
            ->setTitle('Quarterly revenue')
            ->setSeries([['label' => 'revenue ($M)', 'data' => [12.4, 18.1, 21.7, 25.3]]])
            ->setCategoryLabels(['Q1', 'Q2', 'Q3', 'Q4'])
            ->setImageMap([
                ['href' => '/reports/2026q1', 'tooltip' => 'Q1: $12.4M'],
                ['href' => '/reports/2026q2', 'tooltip' => 'Q2: $18.1M'],
                ['href' => '/reports/2026q3', 'tooltip' => 'Q3: $21.7M'],
                ['href' => '/reports/2026q4', 'tooltip' => 'Q4: $25.3M'],
            ]);
    },
];


return ['font' => $font, 'dpi' => $dpi, 'cases' => $cases];
