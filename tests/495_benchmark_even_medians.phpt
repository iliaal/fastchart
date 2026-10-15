--TEST--
Render benchmarks average the two middle samples for even iteration counts and interpolate p90
--FILE--
<?php
namespace FastChartBenchMedianTest;

// Run the actual measurement sections with a deterministic clock and no-op
// renderers. This tests both the calculation and its use in the report.
function hrtime(bool $asNumber): int {
    global $durations, $clockCalls;
    $call = $clockCalls++;
    return $call % 2 ? (int)($durations[intdiv($call, 2) % count($durations)] * 1000000) : 0;
}
function section(string $file, string $start, string $end): string {
    $source = file_get_contents($file);
    $first = strpos($source, $start);
    $last = $first === false ? false : strpos($source, $end, $first);
    if ($first === false || $last === false) throw new \Exception('Missing benchmark section');
    return substr($source, $first, $last - $first);
}
$root = dirname(__DIR__);
$loop = section($root . '/docs/bench/bench.php', 'foreach ($builders as $name => $build)', 'printf("\\n# done');
$function = section($root . '/bench/perf.php', 'function bench(', '/* Repeated digit glyphs');
eval('namespace ' . __NAMESPACE__ . ';' . $function);
$cases = [
    'one' => [[7], 7],
    'two' => [[9, 1], 5],
    'odd' => [[9, 1, 3], 3],
    'even' => [[9, 1, 3, 7], 5],
    'fractional' => [[0.25, 0.75], 0.5],
    'zero' => [[0, 0], 0],
    'ties' => [[7, 1, 7, 7], 7],
    'default50' => [array_merge(array_fill(0, 25, 1), array_fill(0, 25, 9)), 5],
    'default60' => [array_merge(array_fill(0, 30, 1), array_fill(0, 30, 9)), 5],
];
foreach ($cases as $case => [$durations, $expected]) {
    foreach ([0, 3] as $WARMUP) {
        $N = count($durations);
        $W = $H = 1;
        $renders = $clockCalls = 0;
        $builders = ['fixture' => fn($w, $h) => null];
        $encode = function ($chart = null) use (&$renders) { $renders++; return ''; };
        $formats = array_fill_keys(['SVG', 'PNG', 'WebP', 'JPG'], $encode);
        ob_start();
        eval('namespace ' . __NAMESPACE__ . ';' . $loop);
        $actual = ob_get_clean();
        $want = sprintf("%-14s %10.2f %10.2f %10.2f %10.2f\n", 'fixture', $expected, $expected, $expected, $expected);
        $docsOk = $actual === $want && $renders === 4 * ($N + $WARMUP) && $clockCalls === 8 * $N;
        $renders = $clockCalls = 0;
        $result = bench('fixture', $WARMUP, $N, $encode);
        $sorted = $durations;
        sort($sorted);
        $pos = ($N - 1) * 0.9;
        $lo = (int)floor($pos);
        $expected90 = $sorted[$lo] + ($sorted[min($lo + 1, $N - 1)] - $sorted[$lo]) * ($pos - $lo);
        $want = ['name' => 'fixture', 'iters' => $N, 'min' => (float)min($durations),
            'p50' => (float)$expected, 'p90' => (float)$expected90,
            'max' => (float)max($durations), 'mean' => array_sum($durations) / $N, 'out_len' => 0];
        $perfOk = $result == $want && array_keys($result) === array_keys($want)
            && $renders === $N + $WARMUP && $clockCalls === 2 * $N;
        echo "$case warmup=$WARMUP: ", $docsOk && $perfOk ? "ok\n" : "FAIL\n";
    }
}
foreach ([[9, 1, 3, 7], range(1, 10)] as $durations) {
    $clockCalls = 0;
    printf("p90 of %d samples: %.1f\n", count($durations), bench('fixture', 0, count($durations), fn() => '')['p90']);
}
?>
--EXPECT--
one warmup=0: ok
one warmup=3: ok
two warmup=0: ok
two warmup=3: ok
odd warmup=0: ok
odd warmup=3: ok
even warmup=0: ok
even warmup=3: ok
fractional warmup=0: ok
fractional warmup=3: ok
zero warmup=0: ok
zero warmup=3: ok
ties warmup=0: ok
ties warmup=3: ok
default50 warmup=0: ok
default50 warmup=3: ok
default60 warmup=0: ok
default60 warmup=3: ok
p90 of 4 samples: 8.4
p90 of 10 samples: 9.1
