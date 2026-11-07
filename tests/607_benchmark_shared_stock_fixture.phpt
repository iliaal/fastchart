--TEST--
Stock-chart benchmarks preserve identical deterministic candle data across formats
--FILE--
<?php
// Load the scenario factories without starting the render benchmark.
$source = file_get_contents(dirname(__DIR__) . '/bench/perf.php');
$start = strpos($source, '/* Repeated digit glyphs');
$end = strpos($source, '$scenarios = [');
if ($start === false || $end === false || $end <= $start) {
    throw new Exception('Missing benchmark scenario factories');
}
eval(substr($source, $start, $end - $start));

// Preserve the original fixture and the RNG state each factory leaves behind.
mt_srand(424242);
$expected = [];
$price = 100.0;
for ($i = 0; $i < 200; $i++) {
    $open = $price;
    $close = $price + sin($i * 0.3) * 4 + mt_rand(-500, 500) / 250.0;
    $expected[] = [1700000000 + $i * 86400, $open,
        max($open, $close) + 1.5, min($open, $close) - 1.5,
        $close, 1000 + mt_rand(0, 500)];
    $price = $close;
}
$nextRandom = mt_rand();
foreach (['scenario_label_chart', 'scenario_label_chart_jpeg', 'scenario_label_chart_webp'] as $factory) {
    foreach ([1, 17] as $seed) {
        mt_srand($seed);
        $render = $factory();
        $actual = (new ReflectionFunction($render))->getStaticVariables()['rows'];
        if ($actual !== $expected || mt_rand() !== $nextRandom) {
            throw new Exception("Changed fixture or RNG state: $factory");
        }
    }
    echo "$factory: unchanged\n";
}
?>
--EXPECT--
scenario_label_chart: unchanged
scenario_label_chart_jpeg: unchanged
scenario_label_chart_webp: unchanged
