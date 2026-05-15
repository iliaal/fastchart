<?php
/* Canonical case list for the gallery generators. Originally wrote
 * docs/svg-gallery.html (PNG + SVG side-by-side, 21 cases); v1.0
 * promoted docs/v1-gallery.html (SVG / PNG / JPG / WebP four-up)
 * via build-v1-gallery.php, which eval's the prefix of this file
 * for $cases. Running this file directly now only prints summary
 * stats — the rendered HTML output landed in v1-gallery.html.
 *
 * Each case mirrors the construction in docs/examples/ but renders
 * inline so the gallery is self-contained.
 *
 * Run with the fastchart extension loaded — see CLAUDE.md for the
 * explicit -d extension= invocation. */

if (!class_exists('FastChart\\Chart')) {
    fwrite(STDERR, "FastChart not loaded; load fastchart.so + gd.so.\n");
    exit(1);
}

/* Mirror docs/examples/_bootstrap.php so the gallery picks the same
 * default font + DPI the README examples use. Keep the fallback
 * chain short — every entry is a path the example bootstrap also
 * probes. */
function fc_pick_font(): string
{
    $candidates = [
        '/usr/share/fonts/truetype/lato/Lato-Medium.ttf',
        '/usr/share/fonts/truetype/lato/Lato-Regular.ttf',
        '/usr/share/fonts/lato/Lato-Medium.ttf',
        '/usr/share/fonts/lato/Lato-Regular.ttf',
        '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',
        '/usr/share/fonts/dejavu/DejaVuSans.ttf',
        '/usr/share/fonts/TTF/DejaVuSans.ttf',
    ];
    foreach ($candidates as $p) {
        if ($p !== '' && is_readable($p)) return $p;
    }
    return '';
}

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

/* ----------- Build the HTML ----------- */

$rows = '';
$total_svg = 0;
$total_png = 0;
$toc = '';

foreach ($cases as $idx => $case) {
    $c = $case['build']();
    $svg = $c->renderSvg();
    $png = $c->renderPng();
    $total_svg += strlen($svg);
    $total_png += strlen($png);

    $svg_kb = number_format(strlen($svg) / 1024, 1);
    $png_kb = number_format(strlen($png) / 1024, 1);
    $png_b64 = 'data:image/png;base64,' . base64_encode($png);

    $label = htmlspecialchars($case['label'], ENT_QUOTES, 'UTF-8');
    $ref   = htmlspecialchars($case['ref'], ENT_QUOTES, 'UTF-8');
    $code_with_renders = $case['code']
        . "\n\n// construction is identical, only the render call differs:\n"
        . "\$svg = \$c->renderSvg();   // vector\n"
        . "\$png = \$c->renderPng();   // raster";

    /* PHP-side highlighter wraps in <pre><code>...</code></pre>. */
    $code_highlight = highlight_string('<?php ' . "\n" . $code_with_renders, true);
    $code_highlight = preg_replace('/&lt;\?php\s*<br ?\/?>/', '', $code_highlight, 1);

    $n = $idx + 1;
    $toc .= "<li><a href=\"#row-{$n}\">{$label}</a></li>\n";

    $rows .= <<<HTML
<section class="row" id="row-{$n}">
  <h2>{$label}</h2>
  <p class="ref">Source: <a href="https://github.com/iliaal/fastchart/blob/master/{$ref}"><code>{$ref}</code></a></p>
  <div class="code">{$code_highlight}</div>
  <div class="pair">
    <figure>
      <figcaption>PNG (libgd) <span class="size">{$png_kb} KB</span></figcaption>
      <div class="frame"><img src="{$png_b64}" alt="PNG render"></div>
    </figure>
    <figure>
      <figcaption>SVG (vector) <span class="size">{$svg_kb} KB</span></figcaption>
      <div class="frame">{$svg}</div>
    </figure>
  </div>
</section>
HTML;
}

$total_svg_kb = number_format($total_svg / 1024, 1);
$total_png_kb = number_format($total_png / 1024, 1);
$ncharts = count($cases);
$now = date('Y-m-d');
$ver = FastChart\Chart::version();

$html = <<<HTML
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>FastChart {$ver}: PNG vs SVG gallery</title>
<meta name="description" content="Side-by-side PNG (libgd) and SVG output for every fastchart chart family and symbology.">
<style>
:root {
  --bg: #fafafa;
  --fg: #1f2328;
  --muted: #656d76;
  --border: #d0d7de;
  --accent: #0969da;
  --code-bg: #f6f8fa;
}
html, body { margin: 0; padding: 0; background: var(--bg); color: var(--fg);
             font-family: -apple-system, BlinkMacSystemFont, "Segoe UI",
                          "Helvetica Neue", Arial, sans-serif;
             line-height: 1.5; }
header { background: #fff; border-bottom: 1px solid var(--border);
         padding: 20px 24px; }
header h1 { margin: 0 0 6px; font-size: 1.5rem; }
header p { margin: 0; color: var(--muted); font-size: 0.95rem; }
header code { background: var(--code-bg); padding: 1px 6px; border-radius: 4px;
              font-size: 0.85em; font-family: ui-monospace, "SF Mono", Menlo, monospace; }
header a { color: var(--accent); text-decoration: none; }
header a:hover { text-decoration: underline; }
main { max-width: 1200px; margin: 0 auto; padding: 24px; }
nav.toc { background: #fff; border: 1px solid var(--border); border-radius: 8px;
          padding: 14px 20px; margin-bottom: 32px; }
nav.toc h2 { margin: 0 0 10px; font-size: 1rem; color: var(--accent); font-weight: 600; }
nav.toc ol { columns: 2; column-gap: 28px; margin: 0; padding-left: 22px;
             font-size: 0.9rem; }
nav.toc li { break-inside: avoid; margin-bottom: 4px; }
nav.toc a { color: var(--fg); text-decoration: none; }
nav.toc a:hover { color: var(--accent); }
.row { background: #fff; border: 1px solid var(--border); border-radius: 8px;
       margin-bottom: 32px; padding: 20px 24px; scroll-margin-top: 24px; }
.row h2 { margin: 0 0 6px; font-size: 1.05rem; color: var(--accent);
          font-weight: 600; }
.row .ref { margin: 0 0 14px; font-size: 0.85rem; color: var(--muted); }
.row .ref code { font-family: ui-monospace, "SF Mono", Menlo, monospace; }
.code { background: var(--code-bg); border-radius: 6px; padding: 12px 16px;
        margin-bottom: 16px; overflow-x: auto; font-size: 0.85rem;
        line-height: 1.55; border: 1px solid var(--border); }
.code code { font-family: ui-monospace, "SF Mono", Menlo, Consolas, monospace;
             white-space: pre; }
.pair { display: grid; grid-template-columns: 1fr 1fr; gap: 18px; }
@media (max-width: 720px) { .pair { grid-template-columns: 1fr; } }
figure { margin: 0; }
figcaption { font-size: 0.85rem; color: var(--muted); margin-bottom: 6px;
             display: flex; justify-content: space-between; }
figcaption .size { font-family: ui-monospace, "SF Mono", Menlo, monospace; }
.frame { background: #fff; border: 1px solid var(--border); border-radius: 4px;
         overflow: hidden; padding: 10px; display: flex; justify-content: center;
         align-items: center; min-height: 100px; }
.frame svg, .frame img { max-width: 100%; height: auto; display: block; }
footer { max-width: 1200px; margin: 0 auto; padding: 16px 24px 40px;
         color: var(--muted); font-size: 0.9rem; }
footer a { color: var(--accent); }
</style>
</head>
<body>
<header>
  <h1>FastChart {$ver}: PNG vs SVG gallery</h1>
  <p>Side-by-side raster (libgd) and vector (SVG) output for every chart
     family and symbology in the
     <a href="https://github.com/iliaal/fastchart">fastchart</a> PHP
     extension. Each row shows the construction code; the only
     difference between the two outputs is whether you call
     <code>renderPng()</code> or <code>renderSvg()</code> at the end.
     Generated {$now} from <code>scripts/build-readme-gallery.php</code>.
     {$ncharts} charts; PNG total <code>{$total_png_kb} KB</code>,
     SVG total <code>{$total_svg_kb} KB</code>.</p>
</header>
<main>
<nav class="toc">
  <h2>Contents</h2>
  <ol>
{$toc}
  </ol>
</nav>
{$rows}
</main>
<footer>
  Want to render a chart to SVG? Build the same chart object and call
  <code>\$chart-&gt;renderSvg()</code> or
  <code>\$chart-&gt;renderToFile('out.svg')</code>. See the
  <a href="https://github.com/iliaal/fastchart#quick-start">Quick start</a>
  for the full Chart construction pattern, and
  <a href="https://github.com/iliaal/fastchart/blob/master/docs/README.md">docs/README.md</a>
  for the complete example gallery.
</footer>
</body>
</html>
HTML;

/* docs/svg-gallery.html was retired in favor of docs/v1-gallery.html
 * (built by build-v1-gallery.php). This script is now used only as
 * the canonical case list — build-v1-gallery.php evals the prefix
 * up to the "Build the HTML" marker to reuse $cases. Run from the
 * command line still prints summary stats so the case list can be
 * smoke-checked. */
echo "case count: {$ncharts}\n";
echo "SVG total: {$total_svg_kb} KB\n";
echo "PNG total: {$total_png_kb} KB\n";
/* $html / $rows / $toc are retained as locals so the file's tail
 * remains semantically valid even though we no longer write them
 * anywhere. */
