--TEST--
renderToFile rejects an in-place destination change while rasterizing
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (PHP_OS_FAMILY !== 'Linux') die('skip Linux process race test');
if (!function_exists('proc_open')) die('skip proc_open unavailable');
if (!is_readable('/proc/self/maps')) die('skip loaded module path unavailable');
?>
--INI--
asan.detect_leaks=0
--FILE--
<?php

$dir = sys_get_temp_dir() . '/fastchart-inplace-interloper-'
	. bin2hex(random_bytes(6));
mkdir($dir, 0700);
$target = $dir . '/out.png';
file_put_contents($target, 'old');
$inode = fileinode($target);

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
	PHP_BINARY, '-n', '-d', 'extension=' . $module,
	'-r', $childCode, $target,
], [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $pipes);
fclose($pipes[0]);

$tempSeen = false;
$deadline = microtime(true) + 30.0;
while (microtime(true) < $deadline) {
	if (glob($target . '.fctmp-*')) {
		$tempSeen = true;
		break;
	}
	$status = proc_get_status($process);
	if (!$status['running']) break;
	usleep(1000);
}
$changed = $tempSeen
	&& file_put_contents($target, 'concurrent') === 10;
clearstatcache(true, $target);
$sameInode = fileinode($target) === $inode;
$stdout = stream_get_contents($pipes[1]);
$stderr = stream_get_contents($pipes[2]);
fclose($pipes[1]);
fclose($pipes[2]);
$exit = proc_close($process);

echo 'in-place race synchronized: ',
	$changed && $sameInode ? "yes\n" : "NO\n";
echo 'render rejected: ',
	$exit === 3
	&& str_contains($stderr,
		'destination changed while rendering') ? "yes\n" : "NO\n";
echo 'concurrent destination preserved: ',
	is_file($target) && file_get_contents($target) === 'concurrent'
		? "yes\n" : "NO\n";
echo 'temporary cleaned: ',
	glob($target . '.fctmp-*') === [] ? "yes\n" : "NO\n";

foreach (glob($target . '.fctmp-*') as $path) unlink($path);
@unlink($target);
@rmdir($dir);

?>
--EXPECT--
in-place race synchronized: yes
render rejected: yes
concurrent destination preserved: yes
temporary cleaned: yes
