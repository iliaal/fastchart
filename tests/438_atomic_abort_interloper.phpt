--TEST--
renderToFile abort preserves a substituted temporary entry
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (PHP_OS_FAMILY !== 'Linux') die('skip Linux strace fault injection');
if (!function_exists('proc_open')) die('skip proc_open unavailable');
if (!is_readable('/proc/self/maps')) die('skip loaded module path unavailable');
$probe = proc_open([
	'strace', '-qq', '-e', 'trace=write',
	'-e', 'inject=write:error=EIO:when=1',
	'--', 'env', 'ASAN_OPTIONS=detect_leaks=0',
	PHP_BINARY, '-n', '-r', 'fwrite(STDOUT, "probe");',
], [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $pipes);
if (!is_resource($probe)) die('skip strace unavailable');
foreach ($pipes as $pipe) fclose($pipe);
if (proc_close($probe) === 0) die('skip strace write injection ineffective');
?>
--INI--
asan.detect_leaks=0
--FILE--
<?php

$dir = sys_get_temp_dir() . '/fastchart-abort-interloper-'
	. bin2hex(random_bytes(6));
mkdir($dir, 0700);
$target = $dir . '/out.png';

$module = null;
foreach (file('/proc/self/maps') as $mapping) {
	if (preg_match('~(/[^ ]*/fastchart\.so)(?:\s|$)~', $mapping, $match)) {
		$module = $match[1];
		break;
	}
}
if ($module === null) die("loaded module path not found\n");
$childCode = <<<'PHP'
try {
	$chart = (new FastChart\LineChart(4096, 4096))
		->setSeries(array_fill(0, 2048, 50.0));
	$chart->renderToFile($argv[1]);
} catch (Throwable $e) {
	fwrite(STDERR, get_class($e) . ': ' . $e->getMessage() . "\n");
	exit(3);
}
PHP;
$process = proc_open([
	'strace', '-qq', '-e', 'trace=write',
	'-e', 'inject=write:error=EIO:delay_enter=2s:when=1',
	'--', 'env', 'ASAN_OPTIONS=detect_leaks=0',
	PHP_BINARY, '-n', '-d', 'extension=' . $module,
	'-r', $childCode, $target,
], [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $pipes);
fclose($pipes[0]);

$temp = null;
$deadline = microtime(true) + 30.0;
while (microtime(true) < $deadline) {
	$matches = glob($target . '.fctmp-*');
	if ($matches) {
		$temp = $matches[0];
		break;
	}
	$status = proc_get_status($process);
	if (!$status['running']) break;
	usleep(1000);
}
$held = $temp === null ? null : $temp . '.held';
$substituted = $temp !== null
	&& rename($temp, $held)
	&& file_put_contents($temp, 'concurrent') === 10;
$stdout = stream_get_contents($pipes[1]);
$stderr = stream_get_contents($pipes[2]);
fclose($pipes[1]);
fclose($pipes[2]);
$exit = proc_close($process);

echo 'abort race synchronized: ', $substituted ? "yes\n" : "NO\n";
echo 'render rejected: ', $exit === 3 ? "yes\n" : "NO\n";
echo 'concurrent temp preserved: ',
	$temp !== null && is_file($temp)
	&& file_get_contents($temp) === 'concurrent' ? "yes\n" : "NO\n";
echo 'render temp preserved: ',
	$held !== null && is_file($held) ? "yes\n" : "NO\n";
echo 'destination absent: ', !file_exists($target) ? "yes\n" : "NO\n";

foreach ([$temp, $held, $target] as $path) {
	if ($path !== null && (is_file($path) || is_link($path))) unlink($path);
}
rmdir($dir);

?>
--EXPECT--
abort race synchronized: yes
render rejected: yes
concurrent temp preserved: yes
render temp preserved: yes
destination absent: yes
