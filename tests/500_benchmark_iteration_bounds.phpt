--TEST--
Render benchmarks reject empty measurement counts and negative warmup before rendering
--SKIPIF--
<?php
if (!function_exists('proc_open')) die('skip proc_open unavailable');
?>
--FILE--
<?php
$root = dirname(__DIR__);
$scripts = [
    'perf' => ['bench/perf.php', '$ITERS ', 'function bench(', '$ITERS'],
    'docs' => ['docs/bench/bench.php', '$N =', '/* Deterministic data', '$N'],
];
$cases = [
    'unset' => [null, null],
    'empty' => ['', ''],
    'zero iteration default' => ['0', null],
    'zero warmup' => ['2', '0'],
    'positive' => ['2', '1'],
    'legacy integer casts' => ['2.5', '1.5'],
    'negative iterations' => ['-1', '0'],
    'nonnumeric iterations' => ['abc', '0'],
    'zero after cast' => ['0.0', '0'],
    'negative warmup' => ['2', '-1'],
];
foreach ($scripts as $name => [$file, $start, $end, $iterations]) {
    $source = file_get_contents($root . '/' . $file);
    $first = strpos($source, $start);
    $last = $first === false ? false : strpos($source, $end, $first);
    if ($first === false || $last === false) throw new Exception('Missing benchmark configuration');
    $config = substr($source, $first, $last - $first);
    foreach ($cases as $label => [$iters, $warmup]) {
        // Mock getenv so empty and unset remain distinct on every platform.
        $values = ['FC_BENCH_ITERS' => $iters, 'FC_BENCH_WARMUP' => $warmup];
        $setup = 'namespace FastChartBenchBoundsTest; function getenv(string $name): string|false {'
            . '$values = ' . var_export($values, true) . '; return $values[$name] ?? false; }';
        $process = proc_open([
            getenv('TEST_PHP_EXECUTABLE') ?: PHP_BINARY, '-n', '-r',
            $setup . $config . 'echo ' . $iterations . ', ",", $WARMUP, "\n";',
        ], [0 => ['pipe', 'r'], 1 => ['pipe', 'w'], 2 => ['pipe', 'w']], $pipes);
        if (!is_resource($process)) throw new Exception('Could not start PHP');
        fclose($pipes[0]);
        $stdout = stream_get_contents($pipes[1]);
        $stderr = stream_get_contents($pipes[2]);
        fclose($pipes[1]);
        fclose($pipes[2]);
        $status = proc_close($process);
        printf("%s %s: exit=%d stdout=%s stderr=%s\n",
            $name, $label, $status, json_encode($stdout), json_encode($stderr));
    }
}
?>
--EXPECT--
perf unset: exit=0 stdout="60,5\n" stderr=""
perf empty: exit=0 stdout="60,5\n" stderr=""
perf zero iteration default: exit=0 stdout="60,5\n" stderr=""
perf zero warmup: exit=0 stdout="2,0\n" stderr=""
perf positive: exit=0 stdout="2,1\n" stderr=""
perf legacy integer casts: exit=0 stdout="2,1\n" stderr=""
perf negative iterations: exit=1 stdout="" stderr="FC_BENCH_ITERS must be greater than zero\n"
perf nonnumeric iterations: exit=1 stdout="" stderr="FC_BENCH_ITERS must be greater than zero\n"
perf zero after cast: exit=1 stdout="" stderr="FC_BENCH_ITERS must be greater than zero\n"
perf negative warmup: exit=1 stdout="" stderr="FC_BENCH_WARMUP must not be negative\n"
docs unset: exit=0 stdout="50,3\n" stderr=""
docs empty: exit=0 stdout="50,0\n" stderr=""
docs zero iteration default: exit=0 stdout="50,3\n" stderr=""
docs zero warmup: exit=0 stdout="2,0\n" stderr=""
docs positive: exit=0 stdout="2,1\n" stderr=""
docs legacy integer casts: exit=0 stdout="2,1\n" stderr=""
docs negative iterations: exit=1 stdout="" stderr="FC_BENCH_ITERS must be greater than zero\n"
docs nonnumeric iterations: exit=1 stdout="" stderr="FC_BENCH_ITERS must be greater than zero\n"
docs zero after cast: exit=1 stdout="" stderr="FC_BENCH_ITERS must be greater than zero\n"
docs negative warmup: exit=1 stdout="" stderr="FC_BENCH_WARMUP must not be negative\n"
