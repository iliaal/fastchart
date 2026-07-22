--TEST--
renderToFile withdraws output when its parent moves during installation
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (PHP_OS_FAMILY !== 'Linux') die('skip Linux renameat2 test');
if (!function_exists('proc_open')) die('skip proc_open unavailable');
if (!is_readable('/proc/self/maps')) die('skip loaded module path unavailable');
$probe = proc_open([
	'strace', '-qq', '-e', 'trace=renameat2',
	'-e', 'inject=renameat2:delay_enter=1ms:when=1',
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

$root = sys_get_temp_dir() . '/fastchart-basedir-install-'
	. bin2hex(random_bytes(6));
$allowed = $root . '/allowed';
$inside = $allowed . '/inside';
$moved = $root . '/moved';
mkdir($root, 0700);
mkdir($allowed, 0700);
mkdir($inside, 0700);
$target = $inside . '/out.svg';
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
try {
	$chart = (new FastChart\LineChart(640, 360))
		->setSeries(array_fill(0, 1024, 50.0));
	$chart->renderToFile($argv[1]);
} catch (Throwable $e) {
	fwrite(STDERR, get_class($e) . ': ' . $e->getMessage() . "\n");
	exit(3);
}
PHP;
$process = proc_open([
	'strace', '-qq', '-e', 'trace=renameat2',
	'-e', 'inject=renameat2:delay_enter=2s:when=1',
	'--', 'env', 'ASAN_OPTIONS=detect_leaks=0',
	PHP_BINARY, '-n', '-d', 'extension=' . $module,
	'-d', 'open_basedir=' . $allowed,
	'-r', $childCode, $target,
], [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $pipes);
fclose($pipes[0]);

$commitSeen = false;
$deadline = microtime(true) + 30.0;
while (microtime(true) < $deadline) {
	if (glob($inside . '/out.svg.fctmp-*.commit')) {
		$commitSeen = true;
		break;
	}
	$status = proc_get_status($process);
	if (!$status['running']) break;
	usleep(1000);
}
$relocated = $commitSeen && rename($inside, $moved);
$stdout = stream_get_contents($pipes[1]);
$stderr = stream_get_contents($pipes[2]);
fclose($pipes[1]);
fclose($pipes[2]);
$exit = proc_close($process);

echo 'install race synchronized: ', $relocated ? "yes\n" : "NO\n";
echo 'render rejected: ',
	$exit === 3
	&& str_contains($stderr,
		'parent directory moved outside open_basedir') ? "yes\n" : "NO\n";
echo 'prior destination restored: ',
	is_file($moved . '/out.svg')
	&& file_get_contents($moved . '/out.svg') === 'old' ? "yes\n" : "NO\n";
echo 'rendered output withdrawn: ',
	!str_starts_with(file_get_contents($moved . '/out.svg'),
		'<?xml version="1.0"') ? "yes\n" : "NO\n";
echo 'temporary cleaned: ',
	glob($moved . '/out.svg.fctmp-*') === [] ? "yes\n" : "NO\n";

foreach (glob($moved . '/out.svg.fctmp-*') as $path) unlink($path);
@unlink($moved . '/out.svg');
@rmdir($moved);
@rmdir($inside);
@rmdir($allowed);
@rmdir($root);

?>
--EXPECT--
install race synchronized: yes
render rejected: yes
prior destination restored: yes
rendered output withdrawn: yes
temporary cleaned: yes
