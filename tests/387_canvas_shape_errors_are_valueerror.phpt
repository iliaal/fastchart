--TEST--
Remaining render-time canvas shape errors throw ValueError
--EXTENSIONS--
fastchart
--INI--
asan.detect_leaks=0
--FILE--
<?php

function kind(callable $cb): string {
    try {
        $cb();
        return 'none';
    } catch (ValueError $e) {
        return 'ValueError';
    } catch (Error $e) {
        return 'Error';
    }
}

echo "funnel cone: ", kind(function () {
    (new FastChart\Funnel(1890, 210))
        ->setStyle(FastChart\Funnel::STYLE_CONE)
        ->setStages([['label' => 'A', 'value' => 10]])->renderSvg();
}), "\n";

echo "funnel flat: ", kind(function () {
    (new FastChart\Funnel(320, 30))
        ->setStages([
            ['label' => 'A', 'value' => 10],
            ['label' => 'B', 'value' => 9],
            ['label' => 'C', 'value' => 8],
            ['label' => 'D', 'value' => 7],
            ['label' => 'E', 'value' => 6],
            ['label' => 'F', 'value' => 5],
        ])->renderSvg();
}), "\n";

echo "heatmap small: ", kind(function () {
    $row = array_fill(0, 50, 1.0);
    $grid = array_fill(0, 50, $row);
    $grid[49][49] = 2.0;
    (new FastChart\Heatmap(30, 30))->setGrid($grid)->renderSvg();
}), "\n";

echo "calendar small: ", kind(function () {
    (new FastChart\CalendarHeatmap(60, 60))
        ->setData(['2024-01-01' => 1.0, '2024-02-01' => 2.0])
        ->renderSvg();
}), "\n";

echo "calendar narrow: ", kind(function () {
    $d = [];
    $t = strtotime('2024-01-01');
    for ($i = 0; $i < 120; $i++) {
        $d[date('Y-m-d', $t + $i * 7 * 86400)] = 1.0;
    }
    (new FastChart\CalendarHeatmap(120, 140))->setData($d)->renderSvg();
}), "\n";

echo "bullet narrow: ", kind(function () {
    (new FastChart\BulletChart(100, 80))
        ->setRange(0, 100)->setValue(50)->renderSvg();
}), "\n";

?>
--EXPECT--
funnel cone: ValueError
funnel flat: ValueError
heatmap small: ValueError
calendar small: ValueError
calendar narrow: ValueError
bullet narrow: ValueError
