--TEST--
Render benchmark honors zero warmup and initializes output-size drift from the first render
--FILE--
<?php
namespace FastChartBenchWarmupTest;
function getenv(string $name): string|false {
    global $warmup;
    return $name === 'FC_BENCH_WARMUP' ? $warmup : false;
}
function fwrite($stream, string $text): int {
    global $warnings;
    $warnings .= $text;
    return strlen($text);
}
function hrtime(bool $asNumber): int {
    global $clockCalls;
    return ($clockCalls++ % 2) * 1000000;
}
$source = file_get_contents(dirname(__DIR__) . '/bench/perf.php');
$start = strpos($source, '$ITERS ');
$end = strpos($source, 'function bench(');
$endFunction = strpos($source, '/* Repeated digit glyphs');
if ($start === false || $end === false || $endFunction === false) {
    throw new \Exception('Missing benchmark section');
}
$config = substr($source, $start, $end - $start);
eval('namespace ' . __NAMESPACE__ . ';' . substr($source, $end, $endFunction - $end));
foreach ([false, '', '0', '2'] as $warmup) {
    eval('namespace ' . __NAMESPACE__ . ';' . $config);
    $calls = $clockCalls = 0;
    $warnings = '';
    $result = bench('fixture', $WARMUP, 2, function () use (&$calls) { $calls++; return 'abc'; });
    printf("warmup=%s configured=%d renders=%d clocks=%d bytes=%d warnings=%s\n",
        $warmup === false ? 'unset' : ($warmup === '' ? 'empty' : $warmup), $WARMUP, $calls, $clockCalls,
        $result['out_len'], json_encode($warnings));
}
foreach ([[0, ['abc', 'abcd']], [0, ['', 'a']], [1, ['abc', 'abcd', 'abcd']]] as [$warmupCount, $outputs]) {
    $warnings = '';
    $clockCalls = $calls = 0;
    $result = bench('fixture', $warmupCount, 2, function () use (&$calls, $outputs) { return $outputs[$calls++]; });
    printf("drift warmup=%d renders=%d bytes=%d warnings=%s\n", $warmupCount, $calls, $result['out_len'], json_encode($warnings));
}
?>
--EXPECT--
warmup=unset configured=5 renders=7 clocks=4 bytes=3 warnings=""
warmup=empty configured=5 renders=7 clocks=4 bytes=3 warnings=""
warmup=0 configured=0 renders=2 clocks=4 bytes=3 warnings=""
warmup=2 configured=2 renders=4 clocks=4 bytes=3 warnings=""
drift warmup=0 renders=2 bytes=4 warnings="warn: fixture output size drifted at iter 1: 3 -> 4\n"
drift warmup=0 renders=2 bytes=1 warnings="warn: fixture output size drifted at iter 1: 0 -> 1\n"
drift warmup=1 renders=3 bytes=4 warnings="warn: fixture output size drifted at iter 0: 3 -> 4\n"
