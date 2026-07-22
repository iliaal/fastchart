--TEST--
renderToFile fallback preserves a destination replaced after installation
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (PHP_OS_FAMILY !== 'Linux') die('skip Linux strace fault injection');
if (!function_exists('proc_open')) die('skip proc_open unavailable');
if (!is_readable('/proc/self/maps')) die('skip loaded module path unavailable');
$probe = proc_open([
	'strace', '-qq', '-e', 'trace=renameat',
	'-e', 'inject=renameat:delay_exit=1ms:when=1',
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

$dir = sys_get_temp_dir() . '/fastchart-rename-interloper-'
	. bin2hex(random_bytes(6));
mkdir($dir, 0700);
$target = $dir . '/out.svg';
$held = $dir . '/rendered.svg';
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
	'strace', '-qq', '-e', 'trace=renameat,renameat2',
	'-e', 'inject=renameat2:error=EOPNOTSUPP:when=1',
	'-e', 'inject=renameat:delay_exit=2s:when=1',
	'--', 'env', 'ASAN_OPTIONS=detect_leaks=0',
	PHP_BINARY, '-n', '-d', 'extension=' . $module,
	'-r', $childCode, $target,
], [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $pipes);
fclose($pipes[0]);

$installed = false;
$deadline = microtime(true) + 30.0;
while (microtime(true) < $deadline) {
	clearstatcache(true, $target);
	if (is_file($target)
		&& str_starts_with(file_get_contents($target), '<?xml version="1.0"')) {
		$installed = true;
		break;
	}
	$status = proc_get_status($process);
	if (!$status['running']) break;
	usleep(100);
}
$substituted = $installed
	&& rename($target, $held)
	&& file_put_contents($target, 'unrelated') === 9;
$stdout = stream_get_contents($pipes[1]);
$stderr = stream_get_contents($pipes[2]);
fclose($pipes[1]);
fclose($pipes[2]);
$exit = proc_close($process);

$recovery = null;
foreach (glob($target . '.fctmp-*.old') as $candidate) {
	if (is_file($candidate) && file_get_contents($candidate) === 'old') {
		$recovery = $candidate;
		break;
	}
}
echo 'fallback race synchronized: ', $substituted ? "yes\n" : "NO\n";
echo 'render rejected: ',
	$exit === 3
	&& str_contains($stderr,
		'Error: FastChart\\Chart::renderToFile() finalized file changed')
		? "yes\n" : "NO\n";
echo 'concurrent destination preserved: ',
	is_file($target) && file_get_contents($target) === 'unrelated'
		? "yes\n" : "NO\n";
echo 'prior target recoverable: ', $recovery !== null ? "yes\n" : "NO\n";
echo 'installed output moved aside: ',
	is_file($held)
	&& str_starts_with(file_get_contents($held), '<?xml version="1.0"')
		? "yes\n" : "NO\n";

foreach (glob($target . '.fctmp-*') as $path) unlink($path);
foreach ([$target, $held] as $path) {
	if (is_file($path) || is_link($path)) unlink($path);
}
rmdir($dir);

?>
--EXPECT--
fallback race synchronized: yes
render rejected: yes
concurrent destination preserved: yes
prior target recoverable: yes
installed output moved aside: yes
