<?php
/* Build side-by-side SVG vs PNG comparison HTML covering every chart
 * family and symbology. Each row shows the PHP construction code
 * (PHP-highlighted), then SVG (inlined) and PNG (base64 data URI) for
 * the same chart. Output: compare-svg-png.html in the repo root.
 *
 * Run with the fastchart + gd extensions loaded — see CLAUDE.md for
 * the explicit -d extension= invocation. */

if (!class_exists('FastChart\\Chart')) {
    fwrite(STDERR, "FastChart not loaded; load fastchart.so + gd.so on the CLI.\n");
    exit(1);
}

/* Each case: label + PHP source string + build closure that returns
 * the configured chart. Source and closure intentionally hand-mirror
 * each other (rather than eval the string) so the closure can stay
 * statically analysable. */
$cases = [];

/* ============================== LineChart ============================== */

$cases[] = [
    'label' => 'LineChart — multi-series',
    'code'  => <<<'PHP'
$c = (new FastChart\LineChart(480, 280))
    ->setSeries([
        ['label' => 'alpha', 'data' => [10, 14, 12, 18, 22, 19, 25, 23]],
        ['label' => 'beta',  'data' => [ 8, 11, 15, 13, 17, 21, 20, 24]],
    ])
    ->setTitle('Quarterly revenue')
    ->setCategoryLabels(['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug'])
    ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT);
PHP,
    'build' => function () {
        return (new FastChart\LineChart(480, 280))
            ->setSeries([
                ['label' => 'alpha', 'data' => [10, 14, 12, 18, 22, 19, 25, 23]],
                ['label' => 'beta',  'data' => [ 8, 11, 15, 13, 17, 21, 20, 24]],
            ])
            ->setTitle('Quarterly revenue')
            ->setCategoryLabels(['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug'])
            ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT);
    },
];

/* ============================== AreaChart ============================== */

$cases[] = [
    'label' => 'AreaChart — stacked, two series',
    'code'  => <<<'PHP'
$c = (new FastChart\AreaChart(480, 280))
    ->setSeries([
        ['label' => 'A', 'data' => [10, 14, 12, 18, 22, 19, 25, 23]],
        ['label' => 'B', 'data' => [ 8, 11, 15, 13, 17, 21, 20, 24]],
    ])
    ->setCategoryLabels(['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug'])
    ->setTitle('Daily active users')
    ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT);
PHP,
    'build' => function () {
        return (new FastChart\AreaChart(480, 280))
            ->setSeries([
                ['label' => 'A', 'data' => [10, 14, 12, 18, 22, 19, 25, 23]],
                ['label' => 'B', 'data' => [ 8, 11, 15, 13, 17, 21, 20, 24]],
            ])
            ->setCategoryLabels(['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug'])
            ->setTitle('Daily active users')
            ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT);
    },
];

/* ============================== BarChart =============================== */

$cases[] = [
    'label' => 'BarChart — grouped vertical bars',
    'code'  => <<<'PHP'
$c = (new FastChart\BarChart(480, 280))
    ->setSeries([
        ['label' => '2024', 'data' => [42, 58, 67, 71]],
        ['label' => '2025', 'data' => [55, 63, 75, 84]],
    ])
    ->setCategoryLabels(['Q1','Q2','Q3','Q4'])
    ->setTitle('Sales by quarter')
    ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT);
PHP,
    'build' => function () {
        return (new FastChart\BarChart(480, 280))
            ->setSeries([
                ['label' => '2024', 'data' => [42, 58, 67, 71]],
                ['label' => '2025', 'data' => [55, 63, 75, 84]],
            ])
            ->setCategoryLabels(['Q1', 'Q2', 'Q3', 'Q4'])
            ->setTitle('Sales by quarter')
            ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT);
    },
];

$cases[] = [
    'label' => 'BarChart — stacked',
    'code'  => <<<'PHP'
$c = (new FastChart\BarChart(480, 280))
    ->setSeries([
        ['label' => 'web',    'data' => [120, 135, 142, 158, 161]],
        ['label' => 'mobile', 'data' => [ 80, 110, 145, 170, 195]],
        ['label' => 'api',    'data' => [ 40,  55,  70,  88, 105]],
    ])
    ->setStacked(true)
    ->setCategoryLabels(['Q1','Q2','Q3','Q4','Q5'])
    ->setTitle('Stacked bar')
    ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT);
PHP,
    'build' => function () {
        return (new FastChart\BarChart(480, 280))
            ->setSeries([
                ['label' => 'web',    'data' => [120, 135, 142, 158, 161]],
                ['label' => 'mobile', 'data' => [ 80, 110, 145, 170, 195]],
                ['label' => 'api',    'data' => [ 40,  55,  70,  88, 105]],
            ])
            ->setStacked(true)
            ->setCategoryLabels(['Q1', 'Q2', 'Q3', 'Q4', 'Q5'])
            ->setTitle('Stacked bar')
            ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT);
    },
];

/* ============================== PieChart =============================== */

$cases[] = [
    'label' => 'PieChart — five slices',
    'code'  => <<<'PHP'
$c = (new FastChart\PieChart(420, 280))
    ->setSlices([
        'Frontend'      => 38,
        'Backend'       => 27,
        'Database'      => 15,
        'Infrastructure'=> 12,
        'Other'         =>  8,
    ])
    ->setTitle('Engineering time allocation');
PHP,
    'build' => function () {
        return (new FastChart\PieChart(420, 280))
            ->setSlices([
                'Frontend'       => 38,
                'Backend'        => 27,
                'Database'       => 15,
                'Infrastructure' => 12,
                'Other'          =>  8,
            ])
            ->setTitle('Engineering time allocation');
    },
];

$cases[] = [
    'label' => 'PieChart — donut (45% hole)',
    'code'  => <<<'PHP'
$c = (new FastChart\PieChart(420, 280))
    ->setSlices(['A' => 30, 'B' => 25, 'C' => 20, 'D' => 15, 'E' => 10])
    ->setDonutHoleRatio(0.45)
    ->setTitle('Donut chart');
PHP,
    'build' => function () {
        return (new FastChart\PieChart(420, 280))
            ->setSlices(['A' => 30, 'B' => 25, 'C' => 20, 'D' => 15, 'E' => 10])
            ->setDonutHoleRatio(0.45)
            ->setTitle('Donut chart');
    },
];

/* ============================ ScatterChart ============================= */

$cases[] = [
    'label' => 'ScatterChart — with trend line',
    'code'  => <<<'PHP'
$pts = [];
for ($i = 0; $i < 40; $i++) {
    $x = $i * 2.5;
    $y = $x * 0.6 + sin($i / 3) * 8 + (mt_rand(-15, 15) / 10);
    $pts[] = [$x, $y];
}
$c = (new FastChart\ScatterChart(480, 280))
    ->setPoints($pts)
    ->setTrendLine(true)
    ->setTitle('Scatter with linear trend');
PHP,
    'build' => function () {
        mt_srand(1234);
        $pts = [];
        for ($i = 0; $i < 40; $i++) {
            $x = $i * 2.5;
            $y = $x * 0.6 + sin($i / 3) * 8 + (mt_rand(-15, 15) / 10);
            $pts[] = [$x, $y];
        }
        return (new FastChart\ScatterChart(480, 280))
            ->setPoints($pts)
            ->setTrendLine(true)
            ->setTitle('Scatter with linear trend');
    },
];

/* ============================ BubbleChart ============================== */

$cases[] = [
    'label' => 'BubbleChart — three-dim points',
    'code'  => <<<'PHP'
$pts = [
    [10, 20,  8], [25, 18, 14], [40, 35, 20], [55, 28, 12],
    [70, 50, 18], [85, 45,  9], [30, 60, 22], [60, 70, 11],
];
$c = (new FastChart\BubbleChart(480, 280))
    ->setPoints($pts)
    ->setTitle('Reach vs engagement vs cost');
PHP,
    'build' => function () {
        $pts = [
            [10, 20,  8], [25, 18, 14], [40, 35, 20], [55, 28, 12],
            [70, 50, 18], [85, 45,  9], [30, 60, 22], [60, 70, 11],
        ];
        return (new FastChart\BubbleChart(480, 280))
            ->setPoints($pts)
            ->setTitle('Reach vs engagement vs cost');
    },
];

/* ============================ RadarChart =============================== */

$cases[] = [
    'label' => 'RadarChart — filled, two teams',
    'code'  => <<<'PHP'
$c = (new FastChart\RadarChart(420, 380))
    ->setSeries([
        ['label' => 'team A', 'data' => [80, 70, 60, 85, 75]],
        ['label' => 'team B', 'data' => [60, 80, 75, 50, 90]],
    ])
    ->setCategoryLabels(['Speed','Power','Endur','Skill','Mind'])
    ->setFilled(true)
    ->setTitle('Player profile')
    ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT);
PHP,
    'build' => function () {
        return (new FastChart\RadarChart(420, 380))
            ->setSeries([
                ['label' => 'team A', 'data' => [80, 70, 60, 85, 75]],
                ['label' => 'team B', 'data' => [60, 80, 75, 50, 90]],
            ])
            ->setCategoryLabels(['Speed', 'Power', 'Endur', 'Skill', 'Mind'])
            ->setFilled(true)
            ->setTitle('Player profile')
            ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT);
    },
];

/* ============================ PolarChart =============================== */

$cases[] = [
    'label' => 'PolarChart — rose / angular histogram',
    'code'  => <<<'PHP'
// Polar data is [angle_deg, radius] pairs, not flat values.
$pts = [];
for ($i = 0; $i < 12; $i++) {
    $pts[] = [$i * 30.0, 5 + sin($i / 2.0) * 3 + cos($i / 1.3) * 2];
}
$c = (new FastChart\PolarChart(380, 380))
    ->setStyle(FastChart\PolarChart::STYLE_ROSE)
    ->setSeries([['data' => $pts]])
    ->setTitle('Wind direction frequency');
PHP,
    'build' => function () {
        $pts = [];
        for ($i = 0; $i < 12; $i++) {
            $pts[] = [$i * 30.0, 5 + sin($i / 2.0) * 3 + cos($i / 1.3) * 2];
        }
        return (new FastChart\PolarChart(380, 380))
            ->setStyle(FastChart\PolarChart::STYLE_ROSE)
            ->setSeries([['data' => $pts]])
            ->setTitle('Wind direction frequency');
    },
];

/* =========================== SurfaceChart ============================== */

$cases[] = [
    'label' => 'SurfaceChart — 2D grid with color ramp',
    'code'  => <<<'PHP'
$grid = [];
for ($y = 0; $y < 24; $y++) {
    $row = [];
    for ($x = 0; $x < 32; $x++) {
        $row[] = sin($x / 4.0) * cos($y / 3.0) * 10;
    }
    $grid[] = $row;
}
$c = (new FastChart\SurfaceChart(480, 280))
    ->setGrid($grid)
    ->setColorRamp(0x0033CC, 0xFF3300)
    ->setTitle('Field amplitude');
PHP,
    'build' => function () {
        $grid = [];
        for ($y = 0; $y < 24; $y++) {
            $row = [];
            for ($x = 0; $x < 32; $x++) {
                $row[] = sin($x / 4.0) * cos($y / 3.0) * 10;
            }
            $grid[] = $row;
        }
        return (new FastChart\SurfaceChart(480, 280))
            ->setGrid($grid)
            ->setColorRamp(0x0033CC, 0xFF3300)
            ->setTitle('Field amplitude');
    },
];

/* =========================== ContourChart ============================== */

$cases[] = [
    'label' => 'ContourChart — filled isolines',
    'code'  => <<<'PHP'
$grid = [];
for ($y = 0; $y < 24; $y++) {
    $row = [];
    for ($x = 0; $x < 32; $x++) {
        $row[] = sin($x / 5.0) * cos($y / 4.0) * 10;
    }
    $grid[] = $row;
}
$c = (new FastChart\ContourChart(480, 280))
    ->setGrid($grid)
    ->setLevels([-5, 0, 5, 8])
    ->setFilled(true)
    ->setColorRamp(0x2E5CB8, 0xE34A6F)
    ->setTitle('Pressure contours');
PHP,
    'build' => function () {
        $grid = [];
        for ($y = 0; $y < 24; $y++) {
            $row = [];
            for ($x = 0; $x < 32; $x++) {
                $row[] = sin($x / 5.0) * cos($y / 4.0) * 10;
            }
            $grid[] = $row;
        }
        return (new FastChart\ContourChart(480, 280))
            ->setGrid($grid)
            ->setLevels([-5, 0, 5, 8])
            ->setFilled(true)
            ->setColorRamp(0x2E5CB8, 0xE34A6F)
            ->setTitle('Pressure contours');
    },
];

/* ============================ GaugeChart =============================== */

$cases[] = [
    'label' => 'GaugeChart — zoned dial',
    'code'  => <<<'PHP'
$c = (new FastChart\GaugeChart(400, 280))
    ->setRange(0, 100)
    ->setValue(72)
    ->setZones([
        ['from' => 0,  'to' => 50,  'color' => 0x4FB286],
        ['from' => 50, 'to' => 80,  'color' => 0xF4C842],
        ['from' => 80, 'to' => 100, 'color' => 0xE34A6F],
    ])
    ->setTitle('CPU utilization');
PHP,
    'build' => function () {
        return (new FastChart\GaugeChart(400, 280))
            ->setRange(0, 100)
            ->setValue(72)
            ->setZones([
                ['from' => 0,  'to' => 50,  'color' => 0x4FB286],
                ['from' => 50, 'to' => 80,  'color' => 0xF4C842],
                ['from' => 80, 'to' => 100, 'color' => 0xE34A6F],
            ])
            ->setTitle('CPU utilization');
    },
];

/* ============================ GanttChart =============================== */

$cases[] = [
    'label' => 'GanttChart — tasks + dependencies',
    'code'  => <<<'PHP'
$c = (new FastChart\GanttChart(640, 280))
    ->setTitle('Release schedule')
    ->setTasks([
        ['name' => 'Plan',  'start' => 1700000000, 'end' => 1700432000],
        ['name' => 'Build', 'start' => 1700432000, 'end' => 1701036800,
         'depends' => [0]],
        ['name' => 'Test',  'start' => 1700864000, 'end' => 1701123200,
         'depends' => [1], 'color' => 0xFF8800],
        ['name' => 'Ship',  'start' => 1701123200, 'end' => 1701123200,
         'milestone' => true, 'depends' => [2]],
    ]);
PHP,
    'build' => function () {
        return (new FastChart\GanttChart(640, 280))
            ->setTitle('Release schedule')
            ->setTasks([
                ['name' => 'Plan',  'start' => 1700000000, 'end' => 1700432000],
                ['name' => 'Build', 'start' => 1700432000, 'end' => 1701036800, 'depends' => [0]],
                ['name' => 'Test',  'start' => 1700864000, 'end' => 1701123200, 'depends' => [1], 'color' => 0xFF8800],
                ['name' => 'Ship',  'start' => 1701123200, 'end' => 1701123200, 'milestone' => true, 'depends' => [2]],
            ]);
    },
];

/* ============================== BoxPlot ================================ */

$cases[] = [
    'label' => 'BoxPlot — three categories with outliers',
    'code'  => <<<'PHP'
$c = (new FastChart\BoxPlot(480, 320))
    ->setBoxes([
        [10, 25, 35, 45, 60],
        ['min' => 5,  'q1' => 20, 'median' => 30,
         'q3' => 40, 'max' => 55, 'outliers' => [2, 70], 'label' => 'B'],
        [15, 28, 38, 48, 65],
    ])
    ->setCategoryLabels(['A', 'B', 'C'])
    ->setTitle('Latency distribution');
PHP,
    'build' => function () {
        return (new FastChart\BoxPlot(480, 320))
            ->setBoxes([
                [10, 25, 35, 45, 60],
                ['min' => 5,  'q1' => 20, 'median' => 30, 'q3' => 40, 'max' => 55, 'outliers' => [2, 70], 'label' => 'B'],
                [15, 28, 38, 48, 65],
            ])
            ->setCategoryLabels(['A', 'B', 'C'])
            ->setTitle('Latency distribution');
    },
];

/* ============================== Treemap ================================ */

$cases[] = [
    'label' => 'Treemap — proportional rectangles',
    'code'  => <<<'PHP'
$c = (new FastChart\Treemap(480, 320))
    ->setItems([
        ['label' => 'Alpha',   'value' => 100],
        ['label' => 'Bravo',   'value' =>  70],
        ['label' => 'Charlie', 'value' =>  40],
        ['label' => 'Delta',   'value' =>  30, 'color' => 0xFF8800],
        ['label' => 'Echo',    'value' =>  15],
    ])
    ->setTitle('Storage by service');
PHP,
    'build' => function () {
        return (new FastChart\Treemap(480, 320))
            ->setItems([
                ['label' => 'Alpha',   'value' => 100],
                ['label' => 'Bravo',   'value' =>  70],
                ['label' => 'Charlie', 'value' =>  40],
                ['label' => 'Delta',   'value' =>  30, 'color' => 0xFF8800],
                ['label' => 'Echo',    'value' =>  15],
            ])
            ->setTitle('Storage by service');
    },
];

/* =============================== Funnel ================================ */

$cases[] = [
    'label' => 'Funnel — conversion stages',
    'code'  => <<<'PHP'
$c = (new FastChart\Funnel(480, 320))
    ->setStages([
        ['label' => 'Visited',   'value' => 1000],
        ['label' => 'Signed up', 'value' =>  600],
        ['label' => 'Trial',     'value' =>  300],
        ['label' => 'Paid',      'value' =>  120],
    ])
    ->setTitle('Signup funnel');
PHP,
    'build' => function () {
        return (new FastChart\Funnel(480, 320))
            ->setStages([
                ['label' => 'Visited',   'value' => 1000],
                ['label' => 'Signed up', 'value' =>  600],
                ['label' => 'Trial',     'value' =>  300],
                ['label' => 'Paid',      'value' =>  120],
            ])
            ->setTitle('Signup funnel');
    },
];

/* ============================== Waterfall ============================== */

$cases[] = [
    'label' => 'Waterfall — net change breakdown',
    'code'  => <<<'PHP'
$c = (new FastChart\Waterfall(480, 320))
    ->setBars([
        ['label' => 'Start', 'value' => 100, 'kind' => 'total'],
        ['label' => 'Add',   'value' =>  30],
        ['label' => 'Drop',  'value' => -40],
        ['label' => 'Add2',  'value' =>  20],
        ['label' => 'End',   'value' => 110, 'kind' => 'total'],
    ])
    ->setTitle('Revenue movement');
PHP,
    'build' => function () {
        return (new FastChart\Waterfall(480, 320))
            ->setBars([
                ['label' => 'Start', 'value' => 100, 'kind' => 'total'],
                ['label' => 'Add',   'value' =>  30],
                ['label' => 'Drop',  'value' => -40],
                ['label' => 'Add2',  'value' =>  20],
                ['label' => 'End',   'value' => 110, 'kind' => 'total'],
            ])
            ->setTitle('Revenue movement');
    },
];

/* =============================== Heatmap =============================== */

$cases[] = [
    'label' => 'Heatmap — 2D color grid',
    'code'  => <<<'PHP'
$grid = [];
for ($y = 0; $y < 12; $y++) {
    $row = [];
    for ($x = 0; $x < 16; $x++) {
        $row[] = sin($x / 3.0) * cos($y / 2.0) * 5 + 5;
    }
    $grid[] = $row;
}
$c = (new FastChart\Heatmap(520, 320))
    ->setGrid($grid)
    ->setTitle('Activity by hour');
PHP,
    'build' => function () {
        $grid = [];
        for ($y = 0; $y < 12; $y++) {
            $row = [];
            for ($x = 0; $x < 16; $x++) {
                $row[] = sin($x / 3.0) * cos($y / 2.0) * 5 + 5;
            }
            $grid[] = $row;
        }
        return (new FastChart\Heatmap(520, 320))
            ->setGrid($grid)
            ->setTitle('Activity by hour');
    },
];

/* ============================ LinearMeter ============================== */

$cases[] = [
    'label' => 'LinearMeter — horizontal zoned strip',
    'code'  => <<<'PHP'
$c = (new FastChart\LinearMeter(480, 160))
    ->setRange(0, 100)
    ->setValue(72)
    ->setZones([
        ['from' => 0,  'to' => 50,  'color' => 0x4FB286],
        ['from' => 50, 'to' => 80,  'color' => 0xF4C842],
        ['from' => 80, 'to' => 100, 'color' => 0xE34A6F],
    ])
    ->setTitle('Memory usage');
PHP,
    'build' => function () {
        return (new FastChart\LinearMeter(480, 160))
            ->setRange(0, 100)
            ->setValue(72)
            ->setZones([
                ['from' => 0,  'to' => 50,  'color' => 0x4FB286],
                ['from' => 50, 'to' => 80,  'color' => 0xF4C842],
                ['from' => 80, 'to' => 100, 'color' => 0xE34A6F],
            ])
            ->setTitle('Memory usage');
    },
];

/* ============================== StockChart ============================= */

$cases[] = [
    'label' => 'StockChart — candlesticks, SMA, volume',
    'code'  => <<<'PHP'
$rows = [];
$base = 1700000000;
$prices = [
    [100, 110, 95, 108], [108, 112, 102, 105], [105, 115, 100, 113],
    [113, 118, 110, 111], [111, 119, 108, 117], [117, 121, 114, 116],
    [116, 120, 112, 119], [119, 122, 116, 117], [117, 124, 115, 122],
    [122, 126, 119, 125], [125, 130, 121, 128], [128, 132, 123, 127],
    [127, 133, 124, 131], [131, 135, 128, 130], [130, 138, 127, 136],
];
foreach ($prices as $i => $p) {
    $rows[] = [$base + $i * 86400, $p[0], $p[1], $p[2], $p[3], 1000 + $i * 100];
}
$c = (new FastChart\StockChart(640, 360))
    ->setOhlcv($rows)
    ->setMovingAverages([5])
    ->setVolumePane(true)
    ->setTitle('OHLC + SMA(5) + volume')
    ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT);
PHP,
    'build' => function () {
        $rows = [];
        $base = 1700000000;
        $prices = [
            [100, 110,  95, 108], [108, 112, 102, 105], [105, 115, 100, 113],
            [113, 118, 110, 111], [111, 119, 108, 117], [117, 121, 114, 116],
            [116, 120, 112, 119], [119, 122, 116, 117], [117, 124, 115, 122],
            [122, 126, 119, 125], [125, 130, 121, 128], [128, 132, 123, 127],
            [127, 133, 124, 131], [131, 135, 128, 130], [130, 138, 127, 136],
        ];
        foreach ($prices as $i => $p) {
            $rows[] = [$base + $i * 86400, $p[0], $p[1], $p[2], $p[3], 1000 + $i * 100];
        }
        return (new FastChart\StockChart(640, 360))
            ->setOhlcv($rows)
            ->setMovingAverages([5])
            ->setVolumePane(true)
            ->setTitle('OHLC + SMA(5) + volume')
            ->setLegendPosition(FastChart\Chart::LEGEND_TOP_LEFT);
    },
];

/* ============================== Code128 ================================ */

$cases[] = [
    'label' => 'Code128 — 1D linear barcode',
    'code'  => <<<'PHP'
$c = (new FastChart\Code128())
    ->setData('FASTCHART-12345')
    ->setSize(400, 100)
    ->setForeground(0x000000)
    ->setBackground(0xFFFFFF);
PHP,
    'build' => function () {
        return (new FastChart\Code128())
            ->setData('FASTCHART-12345')
            ->setSize(400, 100)
            ->setForeground(0x000000)
            ->setBackground(0xFFFFFF);
    },
];

/* =============================== QrCode ================================ */

$cases[] = [
    'label' => 'QrCode — 2D symbology, ECC level M',
    'code'  => <<<'PHP'
$c = (new FastChart\QrCode())
    ->setData('https://github.com/iliaal/fastchart')
    ->setSize(280, 280)
    ->setEcc(FastChart\QrCode::ECC_M);
PHP,
    'build' => function () {
        return (new FastChart\QrCode())
            ->setData('https://github.com/iliaal/fastchart')
            ->setSize(280, 280)
            ->setEcc(FastChart\QrCode::ECC_M);
    },
];

/* ----------- Render each case + build HTML ----------- */

$rows = '';
$total_svg = 0;
$total_png = 0;

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

    /* Append the divergent render calls so the code block shows
     * exactly where SVG and PNG paths split. Construction is
     * identical; only the final method call differs. */
    $code_with_renders = $case['code'] . "\n\n"
        . "// construction is identical -- only the render call differs:\n"
        . "\$svg = \$c->renderSvg();   // vector output -> SVG column below\n"
        . "\$png = \$c->renderPng();   // raster output -> PNG column below";

    /* highlight_string returns the highlighted source wrapped in
     * <code><span style="color:...">...</span></code>. It also wraps
     * in <pre> when the second arg is true. */
    $code_highlight = highlight_string('<?php ' . "\n" . $code_with_renders, true);
    /* Strip the synthetic <?php opener we prepended for highlighting. */
    $code_highlight = preg_replace('/&lt;\?php\s*<br ?\/?>/', '', $code_highlight, 1);

    $n = $idx + 1;
    $rows .= <<<HTML
<section class="row" id="row-{$n}">
  <h2><span class="num">{$n}.</span> {$label}</h2>
  <div class="code">{$code_highlight}</div>
  <div class="pair">
    <figure>
      <figcaption>SVG <span class="size">{$svg_kb} KB</span></figcaption>
      <div class="frame">{$svg}</div>
    </figure>
    <figure>
      <figcaption>PNG (libgd) <span class="size">{$png_kb} KB</span></figcaption>
      <div class="frame"><img src="{$png_b64}" alt="PNG render"></div>
    </figure>
  </div>
</section>
HTML;
}

/* Index / TOC */
$toc = '';
foreach ($cases as $idx => $case) {
    $n = $idx + 1;
    $label = htmlspecialchars($case['label'], ENT_QUOTES, 'UTF-8');
    $toc .= "<li><a href=\"#row-{$n}\">{$n}. {$label}</a></li>\n";
}

$total_svg_kb = number_format($total_svg / 1024, 1);
$total_png_kb = number_format($total_png / 1024, 1);
$ncharts = count($cases);
$now = date('Y-m-d H:i:s');
$ver = FastChart\Chart::version();

$html = <<<HTML
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>FastChart {$ver}: SVG vs PNG comparison</title>
<style>
:root {
  --bg: #fafafa;
  --fg: #222;
  --muted: #666;
  --border: #ddd;
  --accent: #1f77b4;
  --code-bg: #f7f7f0;
}
html, body { margin: 0; padding: 0; background: var(--bg); color: var(--fg);
             font-family: -apple-system, "Segoe UI", Roboto, sans-serif; }
header { background: #fff; border-bottom: 1px solid var(--border);
         padding: 16px 24px; position: sticky; top: 0; z-index: 10; }
header h1 { margin: 0 0 4px; font-size: 1.4rem; }
header p { margin: 0; color: var(--muted); font-size: 0.9rem; }
header code { background: #f0f0f0; padding: 1px 5px; border-radius: 3px;
              font-size: 0.85em; }
main { max-width: 1200px; margin: 0 auto; padding: 24px; }
nav.toc { background: #fff; border: 1px solid var(--border); border-radius: 8px;
          padding: 12px 18px; margin-bottom: 28px; }
nav.toc h2 { margin: 0 0 8px; font-size: 0.95rem; color: var(--accent); }
nav.toc ol { columns: 2; column-gap: 24px; margin: 0; padding-left: 18px;
             font-size: 0.85rem; }
nav.toc li { break-inside: avoid; margin-bottom: 3px; }
nav.toc a { color: var(--fg); text-decoration: none; }
nav.toc a:hover { color: var(--accent); text-decoration: underline; }
.row { background: #fff; border: 1px solid var(--border); border-radius: 8px;
       margin-bottom: 28px; padding: 16px 20px; scroll-margin-top: 72px; }
.row h2 { margin: 0 0 12px; font-size: 1rem; font-weight: 600;
          color: var(--accent); }
.row h2 .num { color: var(--muted); margin-right: 4px; }
.code { background: var(--code-bg); border-radius: 6px; padding: 10px 14px;
        margin-bottom: 14px; overflow-x: auto; font-size: 0.83rem;
        line-height: 1.5; border: 1px solid #e8e8d8; }
.code code { font-family: ui-monospace, "SF Mono", Menlo, Consolas, monospace;
             white-space: pre; }
.pair { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; }
figure { margin: 0; }
figcaption { font-size: 0.8rem; color: var(--muted); margin-bottom: 6px;
             display: flex; justify-content: space-between; }
figcaption .size { font-family: ui-monospace, "SF Mono", Menlo, monospace; }
.frame { background: #fff; border: 1px solid var(--border); border-radius: 4px;
         overflow: hidden; padding: 8px; display: flex; justify-content: center;
         align-items: center; min-height: 100px; }
.frame svg, .frame img { max-width: 100%; height: auto; display: block; }
footer { max-width: 1200px; margin: 0 auto; padding: 16px 24px 40px;
         color: var(--muted); font-size: 0.85rem; }
</style>
</head>
<body>
<header>
  <h1>FastChart {$ver}: SVG vs PNG side-by-side</h1>
  <p>Every chart family + symbology rendered through both backends.
     Each row shows the construction code, then SVG (inlined) and PNG
     (base64 data URI). Generated {$now}.
     SVG total: <code>{$total_svg_kb} KB</code> &middot;
     PNG total: <code>{$total_png_kb} KB</code> &middot;
     {$ncharts} charts.</p>
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
  Generated by <code>scripts/build-svg-compare.php</code>.
  SVG support covers all chart families (LineChart, AreaChart,
  BarChart, PieChart, ScatterChart, BubbleChart, RadarChart,
  PolarChart, SurfaceChart, ContourChart, GaugeChart, GanttChart,
  BoxPlot, Treemap, Funnel, Waterfall, Heatmap, LinearMeter,
  StockChart) and the Symbol family (Code128, QrCode).
</footer>
</body>
</html>
HTML;

$out = __DIR__ . '/../compare-svg-png.html';
file_put_contents($out, $html);
echo "wrote ", realpath($out),
     " (", number_format(filesize($out) / 1024, 1), " KB)\n";
echo "SVG total: {$total_svg_kb} KB across {$ncharts} charts\n";
echo "PNG total: {$total_png_kb} KB across {$ncharts} charts\n";
