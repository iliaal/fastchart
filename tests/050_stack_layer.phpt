--TEST--
BarChart::setStackMode(STACK_LAYER) layers translucent bars at the same baseline
--EXTENSIONS--
fastchart
--FILE--
<?php

$bytes = (new FastChart\BarChart(600, 400))
    ->setStacked(true)
    ->setStackMode(FastChart\Chart::STACK_LAYER)
    ->setSeries([
        ['data' => [10, 25, 18, 30, 22], 'label' => 'A'],
        ['data' => [15, 18, 22, 28, 18], 'label' => 'B'],
        ['data' => [8,  20, 16, 35, 12], 'label' => 'C'],
    ])
    ->renderPng();
echo "renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

// STACK_BESIDE alias falls through to setStacked(false).
$bytes2 = (new FastChart\BarChart(600, 400))
    ->setStacked(true)
    ->setStackMode(FastChart\Chart::STACK_BESIDE)
    ->setSeries([
        ['data' => [10, 20, 30], 'label' => 'A'],
        ['data' => [15, 25, 35], 'label' => 'B'],
    ])
    ->renderPng();
echo "beside_renders: ", strlen($bytes2) > 1024 ? "ok" : "bad", "\n";

// STACK_SUM is the default (cumulative).
$bytes3 = (new FastChart\BarChart(600, 400))
    ->setStacked(true)
    ->setStackMode(FastChart\Chart::STACK_SUM)
    ->setSeries([
        ['data' => [10, 20, 30]],
        ['data' => [15, 25, 35]],
    ])
    ->renderPng();
echo "sum_renders: ", strlen($bytes3) > 1024 ? "ok" : "bad", "\n";

// Out-of-range rejected.
try {
    (new FastChart\BarChart)->setStackMode(99);
    echo "bad: no throw\n";
} catch (\ValueError $e) {
    echo "bad: ValueError ok\n";
}
?>
--EXPECT--
renders: ok
beside_renders: ok
sum_renders: ok
bad: ValueError ok
