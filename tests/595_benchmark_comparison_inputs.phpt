--TEST--
Benchmark comparison validates complete matching reports and handles zero baselines
--SKIPIF--
<?php
if (!function_exists('proc_open')) die('skip proc_open unavailable');
?>
--FILE--
<?php
$script = dirname(__DIR__) . '/bench/diff.php';
$basePath = __DIR__ . '/595_base.json';
$candidatePath = __DIR__ . '/595_candidate.json';
function compareReports($base, $candidate, int $missing = 0): array {
    global $script, $basePath, $candidatePath;
    file_put_contents($basePath, is_string($base) ? $base : json_encode($base));
    file_put_contents($candidatePath, is_string($candidate) ? $candidate : json_encode($candidate));
    if ($missing) unlink($missing === 1 ? $basePath : $candidatePath);
    $process = proc_open([getenv('TEST_PHP_EXECUTABLE') ?: PHP_BINARY, '-n', $script, $basePath, $candidatePath],
        [0 => ['pipe', 'r'], 1 => ['pipe', 'w'], 2 => ['pipe', 'w']], $pipes);
    if (!is_resource($process)) throw new Exception('Could not start PHP');
    fclose($pipes[0]);
    $out = stream_get_contents($pipes[1]);
    $err = stream_get_contents($pipes[2]);
    fclose($pipes[1]); fclose($pipes[2]);
    return [proc_close($process), $out, $err];
}
$row = ['p50' => 10, 'min' => 5, 'out_len' => 20];
$base = ['label' => 'base', 'results' => ['a' => $row, 'b' => $row]];
$bad = [
    'malformed JSON' => '{', 'null' => 'null', 'scalar' => '42',
    'no results' => ['label' => 'bad'], 'empty results' => ['results' => []],
    'missing scenario' => ['results' => ['a' => $row]],
    'extra scenario' => ['results' => ['a' => $row, 'b' => $row, 'c' => $row]],
    'list results' => ['results' => [$row, $row]],
    'bad label' => array_replace($base, ['label' => []]),
];
foreach (['p50', 'min', 'out_len'] as $field) {
    $report = $base;
    unset($report['results']['b'][$field]);
    $bad["$field missing"] = $report;
    foreach (['null' => null, 'negative' => -1, 'string' => '1', 'bool' => true, 'array' => []] as $kind => $value) {
        $report = $base;
        $report['results']['b'][$field] = $value;
        $bad["$field $kind"] = $report;
    }
}
$bad['fractional length'] = ['results' => ['a' => $row, 'b' => array_replace($row, ['out_len' => 1.5])]];
$bad['infinite time'] = '{"results":{"a":{"p50":1e400,"min":1,"out_len":1},"b":{"p50":1,"min":1,"out_len":1}}}';
foreach ($bad as $label => $report) {
    foreach ([false, true] as $reverse) {
        [$status, $out, $err] = $reverse ? compareReports($report, $base) : compareReports($base, $report);
        if ($status !== 1 || $out !== '' || $err === '' || str_contains($err, 'Fatal error') || str_contains($err, 'Warning')) {
            echo "FAIL $label reverse=$reverse\n";
        }
    }
}
foreach ([1, 2] as $missing) {
    [$status, $out, $err] = compareReports($base, $base, $missing);
    if ($status !== 1 || $out !== '' || !str_starts_with($err, 'Cannot read report: ')) echo "FAIL missing file\n";
}
echo "invalid reports checked\n";
$candidate = ['label' => 'candidate', 'results' => [
    'b' => ['p50' => 5, 'min' => 10, 'out_len' => 21],
    'a' => ['p50' => 20, 'min' => 2.5, 'out_len' => 20],
]];
[$status, $out, $err] = compareReports($base, $candidate);
$expected = sprintf("baseline: base    candidate: candidate\n%-14s | %10s %10s | %10s %10s | %8s %8s | %s\n",
    'scenario', 'base p50', 'cand p50', 'base min', 'cand min', 'delta%', 'min%', 'out_size_ok')
    . str_repeat('-', 110) . "\n"
    . sprintf("%-14s | %10.2f %10.2f | %10.2f %10.2f | %+7.1f%% %+7.1f%% | same\n", 'a', 10, 20, 5, 2.5, 100, -50)
    . sprintf("%-14s | %10.2f %10.2f | %10.2f %10.2f | %+7.1f%% %+7.1f%% | 20→21\n", 'b', 10, 5, 5, 10, -50, 100);
echo $status === 0 && $err === '' && $out === $expected ? "complete normal output unchanged\n" : "FAIL normal output\n";
foreach ([0, 1] as $value) {
    $zero = ['results' => ['zero' => ['p50' => 0, 'min' => 0, 'out_len' => 0]]];
    $candidate = ['results' => ['zero' => ['p50' => $value, 'min' => $value, 'out_len' => 0]]];
    [$status, $out, $err] = compareReports($zero, $candidate);
    $line = sprintf("%-14s | %10.2f %10.2f | %10.2f %10.2f | %8s %8s | same\n", 'zero', 0, $value, 0, $value, 'n/a', 'n/a');
    echo $status === 0 && $err === '' && str_ends_with($out, $line) ? "zero to $value: n/a\n" : "FAIL zero\n";
}
$tiny = ['results' => ['tiny' => ['p50' => 1e-300, 'min' => 1e-300, 'out_len' => 0]]];
$huge = ['results' => ['tiny' => ['p50' => 1e300, 'min' => 1e300, 'out_len' => 0]]];
[$status, $out, $err] = compareReports($tiny, $huge);
echo $status === 0 && $err === '' && substr_count($out, 'n/a') === 2 ? "unrepresentable change: n/a\n" : "FAIL overflow\n";
unlink($basePath); unlink($candidatePath);
?>
--CLEAN--
<?php
@unlink(__DIR__ . '/595_base.json');
@unlink(__DIR__ . '/595_candidate.json');
?>
--EXPECT--
invalid reports checked
complete normal output unchanged
zero to 0: n/a
zero to 1: n/a
unrepresentable change: n/a
