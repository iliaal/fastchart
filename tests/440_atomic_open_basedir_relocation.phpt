--TEST--
renderToFile rejects a pinned parent moved outside open_basedir
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (PHP_OS_FAMILY !== 'Linux') die('skip Linux pinned-directory test');
if (!function_exists('proc_open')) die('skip proc_open unavailable');
if (!is_readable('/proc/self/maps')) die('skip loaded module path unavailable');
?>
--INI--
asan.detect_leaks=0
--FILE--
<?php

$root = sys_get_temp_dir() . '/fastchart-basedir-relocation-'
	. bin2hex(random_bytes(6));
$allowed = $root . '/allowed';
$inside = $allowed . '/inside';
$moved = $root . '/moved';
mkdir($root, 0700);
mkdir($allowed, 0700);
mkdir($inside, 0700);
$target = $inside . '/out.png';

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
	'-d', 'open_basedir=' . $allowed,
	'-r', $childCode, $target,
], [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $pipes);
fclose($pipes[0]);

$tempSeen = false;
$deadline = microtime(true) + 30.0;
while (microtime(true) < $deadline) {
	if (glob($inside . '/out.png.fctmp-*')) {
		$tempSeen = true;
		break;
	}
	$status = proc_get_status($process);
	if (!$status['running']) break;
	usleep(1000);
}
$relocated = $tempSeen && rename($inside, $moved);
$stdout = stream_get_contents($pipes[1]);
$stderr = stream_get_contents($pipes[2]);
fclose($pipes[1]);
fclose($pipes[2]);
$exit = proc_close($process);

echo 'relocation race synchronized: ', $relocated ? "yes\n" : "NO\n";
echo 'render rejected: ',
	$exit === 3
	&& str_contains($stderr,
		'parent directory is outside open_basedir') ? "yes\n" : "NO\n";
echo 'outside output absent: ',
	!file_exists($moved . '/out.png') ? "yes\n" : "NO\n";
echo 'temporary cleaned: ',
	glob($moved . '/out.png.fctmp-*') === [] ? "yes\n" : "NO\n";

foreach (glob($moved . '/out.png.fctmp-*') as $path) unlink($path);
@unlink($moved . '/out.png');
@rmdir($moved);
@rmdir($inside);
@rmdir($allowed);
@rmdir($root);

?>
--EXPECT--
relocation race synchronized: yes
render rejected: yes
outside output absent: yes
temporary cleaned: yes
