--TEST--
setYAxisLabelFormat / setXAxisLabelFormat apply sprintf to tick labels
--EXTENSIONS--
fastchart
--FILE--
<?php

// Y format: dollar prefix should appear in rendered output.
$bytes = (new FastChart\LineChart(500, 400))
    ->setYAxisLabelFormat('$%.2f')
    ->setSeries([1.0, 5.0, 3.0, 8.0, 4.0])
    ->renderPng();
echo "y_format_renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

// Setter validation: embedded NUL rejected.
try {
    (new FastChart\LineChart)->setYAxisLabelFormat("\$%.2f\0evil");
    echo "nul: no throw\n";
} catch (\ValueError $e) {
    echo "nul: ValueError ok\n";
}

// Empty string clears.
$out = (new FastChart\LineChart)
    ->setYAxisLabelFormat('$%.2f')
    ->setYAxisLabelFormat('');
var_dump($out instanceof FastChart\LineChart);

// X-axis numeric format on Stock chart.
$rows = [];
for ($i = 0; $i < 5; $i++) {
    $rows[] = [1700000000 + $i * 86400, 100, 101, 99, 100];
}
$bytes_x = (new FastChart\StockChart(600, 400))
    ->setOhlcv($rows)
    ->setXAxisLabelFormat('%.0f')
    ->renderPng();
echo "x_format_renders: ", strlen($bytes_x) > 1024 ? "ok" : "bad", "\n";
?>
--EXPECT--
y_format_renders: ok
nul: ValueError ok
bool(true)
x_format_renders: ok
