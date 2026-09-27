--TEST--
StockChart SMA and WMA preserve finite averages after outliers and at the float limit
--EXTENSIONS--
fastchart
--FILE--
<?php
function moving_average_lines(array $closes, int $type, float $min, float $max, int $period = 2): array {
    $rows = [];
    foreach ($closes as $i => $close) {
        $rows[] = [1700000000 + $i * 86400, $close, $close, $close, $close];
    }
    $svg = (new FastChart\StockChart(500, 400))
        ->setOhlcv($rows)
        ->setPlotRect(60, 20, 460, 320)
        ->setYAxisRange($min, $max)
        ->setLegendPosition(FastChart\Chart::LEGEND_NONE)
        ->addMovingAverage($period, $type)
        ->renderSvg();
    preg_match_all('/<line x1="[-0-9.]+" y1="([-0-9.]+)" x2="[-0-9.]+" y2="([-0-9.]+)" stroke="#1F77B4" stroke-width="2"/', $svg, $matches, PREG_SET_ORDER);
    return array_map(static fn(array $m): array => [(float)$m[1], (float)$m[2]], $matches);
}

foreach (['SMA' => FastChart\StockChart::MA_SMA, 'WMA' => FastChart\StockChart::MA_WMA] as $name => $type) {
    $lines = moving_average_lines([1e15, 100, 100, 100, 100, 100], $type, 99.99, 100.01);
    $ok = count($lines) === 4;
    foreach (array_slice($lines, 1) as [$y1, $y2]) {
        $ok = $ok && $y1 === 170.0 && $y2 === 170.0;
    }
    echo "$name departing outlier: ", $ok ? 'ok' : 'BAD', "\n";

    $lines = moving_average_lines([96, 98, 100, 102, 104], $type, 90, 110);
    $values = $name === 'SMA' ? [97, 99, 101, 103] : [292 / 3, 298 / 3, 304 / 3, 310 / 3];
    $ys = array_map(static fn(float $v): int => 320 - (int)(($v - 90) * 15 + 0.5), $values);
    $ok = count($lines) === 3;
    foreach ($lines as $i => [$y1, $y2]) {
        $ok = $ok && $y1 === (float)$ys[$i] && $y2 === (float)$ys[$i + 1];
    }
    echo "$name ordinary: ", $ok ? 'ok' : 'BAD', "\n";

    foreach ([$name === 'SMA' ? 105 : 14, 3, 4093] as $period) {
        foreach ([1, -1] as $sign) {
            $value = $sign * PHP_FLOAT_MAX;
            [$min, $max, $want] = $sign === 1
                ? [PHP_FLOAT_MAX / 2, PHP_FLOAT_MAX, 20.0]
                : [-PHP_FLOAT_MAX, -PHP_FLOAT_MAX / 2, 320.0];
            $lines = moving_average_lines(array_fill(0, $period + 3, $value), $type, $min, $max, $period);
            $ok = count($lines) === 3;
            foreach ($lines as [$y1, $y2]) {
                $ok = $ok && $y1 === $want && $y2 === $want;
            }
            echo "$name period $period sign $sign float limit: ", $ok ? 'ok' : 'BAD', "\n";
        }
    }
}
?>
--EXPECT--
SMA departing outlier: ok
SMA ordinary: ok
SMA period 105 sign 1 float limit: ok
SMA period 105 sign -1 float limit: ok
SMA period 3 sign 1 float limit: ok
SMA period 3 sign -1 float limit: ok
SMA period 4093 sign 1 float limit: ok
SMA period 4093 sign -1 float limit: ok
WMA departing outlier: ok
WMA ordinary: ok
WMA period 14 sign 1 float limit: ok
WMA period 14 sign -1 float limit: ok
WMA period 3 sign 1 float limit: ok
WMA period 3 sign -1 float limit: ok
WMA period 4093 sign 1 float limit: ok
WMA period 4093 sign -1 float limit: ok
