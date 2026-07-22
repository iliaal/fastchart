--TEST--
renderToFile rejects commit staging replaced before identity validation
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (PHP_OS_FAMILY !== 'Linux') die('skip Linux strace fault injection');
if (!function_exists('proc_open')) die('skip proc_open unavailable');
if (!is_readable('/proc/self/maps')) die('skip loaded module path unavailable');
$probe = proc_open([
	'strace', '-qq', '-e', 'trace=linkat',
	'-e', 'inject=linkat:delay_exit=1ms:when=1',
	'--', 'true',
], [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $pipes);
if (!is_resource($probe)) die('skip strace unavailable');
foreach ($pipes as $pipe) fclose($pipe);
if (proc_close($probe) !== 0) die('skip strace injection unavailable');
?>
--INI--
asan.detect_leaks=0
--FILE--
<?php

$dir = sys_get_temp_dir() . '/fastchart-stage-precheck-'
	. bin2hex(random_bytes(6));
mkdir($dir, 0700);
$target = $dir . '/out.svg';
file_put_contents($target, 'old');

$module = null;
foreach (file('/proc/self/maps') as $mapping) {
	if (preg_match('~(/[^ ]*/fastchart\.so)(?:\s|$)~', $mapping, $match)) {
		$module = $match[1];
		break;
	}
}
if ($module === null) die("loaded module path not found\n");
$childCode = <<<'PHP'
$chart = (new FastChart\LineChart(640, 360))
	->setSeries(array_fill(0, 1024, 50.0));
try {
	echo $chart->renderToFile($argv[1]), "\n";
} catch (Throwable $e) {
	fwrite(STDERR, get_class($e) . ': ' . $e->getMessage() . "\n");
	exit(3);
}
PHP;
$process = proc_open([
	'strace', '-qq', '-e', 'trace=linkat',
	'-e', 'inject=linkat:delay_exit=2s:when=1',
	'--', 'env', 'ASAN_OPTIONS=detect_leaks=0',
	PHP_BINARY, '-n', '-d', 'extension=' . $module,
	'-r', $childCode, $target,
], [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $pipes);
fclose($pipes[0]);

$commit = null;
$deadline = microtime(true) + 30.0;
while (microtime(true) < $deadline) {
	$matches = glob($target . '.fctmp-*.commit');
	if ($matches) {
		$commit = $matches[0];
		break;
	}
	$status = proc_get_status($process);
	if (!$status['running']) break;
	usleep(100);
}
$held = $commit === null ? null : $commit . '.held';
$substituted = $commit !== null
	&& rename($commit, $held)
	&& file_put_contents($commit, 'unrelated') === 9;
$stdout = stream_get_contents($pipes[1]);
$stderr = stream_get_contents($pipes[2]);
fclose($pipes[1]);
fclose($pipes[2]);
$exit = proc_close($process);

echo 'precheck race synchronized: ', $substituted ? "yes\n" : "NO\n";
echo 'render rejected: ',
	$exit === 3
	&& str_contains($stderr, 'could not secure finalization') ? "yes\n" : "NO\n";
echo 'concurrent stage preserved: ',
	$commit !== null && is_file($commit)
	&& file_get_contents($commit) === 'unrelated' ? "yes\n" : "NO\n";
echo 'rendered stage preserved: ',
	$held !== null && is_file($held)
	&& str_starts_with(file_get_contents($held), '<?xml version="1.0"')
		? "yes\n" : "NO\n";
echo 'destination preserved: ',
	is_file($target) && file_get_contents($target) === 'old' ? "yes\n" : "NO\n";

foreach ([$commit, $held, $target] as $path) {
	if ($path !== null && (is_file($path) || is_link($path))) unlink($path);
}
rmdir($dir);

?>
--EXPECT--
precheck race synchronized: yes
render rejected: yes
concurrent stage preserved: yes
rendered stage preserved: yes
destination preserved: yes
