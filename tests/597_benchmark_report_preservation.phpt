--TEST--
Benchmark runner preserves saved reports on failure and publishes only successful output
--SKIPIF--
<?php
if (PHP_OS_FAMILY === 'Windows') die('skip Bash runner is Unix-only');
if (!function_exists('proc_open')) die('skip proc_open unavailable');
$p = proc_open(['bash', '-c', 'command -v mktemp >/dev/null'], [0=>['pipe','r'],1=>['pipe','w'],2=>['pipe','w']], $pipes);
if (!is_resource($p)) die('skip bash unavailable');
foreach ($pipes as $pipe) fclose($pipe);
if (proc_close($p) !== 0) die('skip bash or mktemp unavailable');
?>
--FILE--
<?php
$root = sys_get_temp_dir() . '/fastchart bench ' . bin2hex(random_bytes(8));
mkdir($root);
mkdir("$root/bench");
mkdir("$root/modules");
mkdir("$root/bench/results");
copy(dirname(__DIR__) . '/bench/run.sh', "$root/bench/run.sh");
file_put_contents("$root/modules/fastchart.so", 'fixture');
$mock = "$root/mock php";
file_put_contents($mock, <<<'SH'
#!/usr/bin/env bash
printf 'diagnostic\n' >&2
printf '{"label":"%s","iters":"%s"}\n' "$FC_BENCH_LABEL" "$FC_BENCH_ITERS"
exit "${FC_TEST_STATUS:-0}"
SH
);
chmod($mock, 0700);
function runFixture(string $label, string $status, array $extra = []): array {
    global $root, $mock;
    $environment = getenv();
    $environment['FC_BENCH_PHP'] = $mock;
    $environment['FC_TEST_STATUS'] = $status;
    $environment = array_merge($environment, $extra);
    $p = proc_open(['bash', "$root/bench/run.sh", $label, '7'],
        [0=>['pipe','r'],1=>['pipe','w'],2=>['pipe','w']], $pipes, null, $environment);
    if (!is_resource($p)) throw new Exception('Cannot run benchmark fixture');
    fclose($pipes[0]);
    $out = stream_get_contents($pipes[1]);
    $err = stream_get_contents($pipes[2]);
    fclose($pipes[1]); fclose($pipes[2]);
    return [proc_close($p), $out, $err];
}
function check(bool $condition, string $label): void {
    if (!$condition) throw new Exception($label);
    echo "$label: ok\n";
}
try {
    $report = "$root/bench/results/saved report.json";
    file_put_contents($report, "previous report\n");
    [$code, $out, $err] = runFixture('saved report', '23');
    check($code === 23 && $out === '' && $err === "diagnostic\n"
        && file_get_contents($report) === "previous report\n", 'failed replacement preserves report and status');
    [$code, $out, $err] = runFixture('new report', '23');
    check($code === 23 && $out === '' && !file_exists("$root/bench/results/new report.json"),
        'failed first run publishes nothing');
    [$code, $out, $err] = runFixture('saved report', '0');
    check($code === 0 && $out === "wrote $report\n" && $err === "diagnostic\n"
        && file_get_contents($report) === "{\"label\":\"saved report\",\"iters\":\"7\"}\n",
        'success replaces report with complete output');
    [$code, $out, $err] = runFixture('new report', '0');
    check($code === 0 && file_get_contents("$root/bench/results/new report.json")
        === "{\"label\":\"new report\",\"iters\":\"7\"}\n", 'success creates new report');
    mkdir("$root/bin");
    file_put_contents("$root/bin/mv", "#!/usr/bin/env bash\nprintf 'rename failed\\n' >&2\nexit 31\n");
    chmod("$root/bin/mv", 0700);
    $saved = file_get_contents($report);
    [$code, $out, $err] = runFixture('saved report', '0', ['PATH' => "$root/bin:" . getenv('PATH')]);
    check($code === 31 && $out === '' && $err === "diagnostic\nrename failed\n"
        && file_get_contents($report) === $saved, 'rename failure preserves report');
    file_put_contents("$root/target", "target remains\n");
    symlink("$root/target", "$root/bench/results/link.json");
    [$code, $out, $err] = runFixture('link', '0');
    check($code === 0 && !is_link("$root/bench/results/link.json")
        && file_get_contents("$root/target") === "target remains\n"
        && file_get_contents("$root/bench/results/link.json") === "{\"label\":\"link\",\"iters\":\"7\"}\n",
        'symlink replaced without changing its target');
    mkdir("$root/bench/results/directory.json");
    [$code, $out, $err] = runFixture('directory', '0');
    check($code !== 0 && $out === '' && strpos($err, 'report path is a directory: ') === 0
        && glob("$root/bench/results/directory.json/*") === [], 'directory target rejected');
    check(glob("$root/bench/results/*.tmp.*") === [], 'temporary files cleaned');
} finally {
    $iterator = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($root,
        FilesystemIterator::SKIP_DOTS), RecursiveIteratorIterator::CHILD_FIRST);
    foreach ($iterator as $file) {
        $file->isDir() ? rmdir($file->getPathname()) : unlink($file->getPathname());
    }
    rmdir($root);
}
?>
--EXPECT--
failed replacement preserves report and status: ok
failed first run publishes nothing: ok
success replaces report with complete output: ok
success creates new report: ok
rename failure preserves report: ok
symlink replaced without changing its target: ok
directory target rejected: ok
temporary files cleaned: ok
