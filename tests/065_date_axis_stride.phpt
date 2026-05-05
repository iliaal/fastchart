--TEST--
setDateAxisStride: calendar-aware tick spacing on time-axis charts
--EXTENSIONS--
fastchart
--FILE--
<?php

$rows = [];
for ($i = 0; $i < 60; $i++) {
    $rows[] = [1700000000 + $i * 86400, 100, 102, 99, 101];
}

foreach ([
    'DAY'     => FastChart\Chart::DATE_DAY,
    'WEEK'    => FastChart\Chart::DATE_WEEK,
    'MONTH'   => FastChart\Chart::DATE_MONTH,
    'QUARTER' => FastChart\Chart::DATE_QUARTER,
    'YEAR'    => FastChart\Chart::DATE_YEAR,
] as $name => $unit) {
    $b = (new FastChart\StockChart(700, 400))
        ->setOhlcv($rows)
        ->setDateAxisStride($unit, 1)
        ->renderPng();
    echo "$name: ", strlen($b) > 1024 ? "ok" : "bad", "\n";
}

// every=0 means auto (the legacy default).
$b = (new FastChart\StockChart(700, 400))
    ->setOhlcv($rows)
    ->setDateAxisStride(FastChart\Chart::DATE_DAY, 0)
    ->renderPng();
echo "auto: ", strlen($b) > 1024 ? "ok" : "bad", "\n";

// Out-of-range unit.
try {
    (new FastChart\StockChart)->setDateAxisStride(99, 1);
    echo "bad_unit: no throw\n";
} catch (\ValueError $e) {
    echo "bad_unit: ValueError ok\n";
}
try {
    (new FastChart\StockChart)->setDateAxisStride(FastChart\Chart::DATE_DAY, 5000);
    echo "bad_every: no throw\n";
} catch (\ValueError $e) {
    echo "bad_every: ValueError ok\n";
}
?>
--EXPECT--
DAY: ok
WEEK: ok
MONTH: ok
QUARTER: ok
YEAR: ok
auto: ok
bad_unit: ValueError ok
bad_every: ValueError ok
