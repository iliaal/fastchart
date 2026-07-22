--TEST--
renderToFile fallback links the opened temporary file after path substitution
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (PHP_OS_FAMILY !== 'Linux') die('skip Linux strace fault injection');
if (!function_exists('proc_open')) die('skip proc_open unavailable');
if (!is_readable('/proc/self/maps')) die('skip loaded module path unavailable');
$probe = proc_open([
	'strace', '-qq', '-e', 'trace=linkat',
	'-e', 'inject=linkat:delay_enter=1ms:when=1',
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

$dir = sys_get_temp_dir() . '/fastchart-new-fallback-race-'
	. bin2hex(random_bytes(6));
mkdir($dir, 0700);
$target = $dir . '/out.svg';
$victim = $dir . '/victim';
file_put_contents($victim, 'secret');
chmod($victim, 0600);

$module = null;
foreach (file('/proc/self/maps') as $mapping) {
	if (preg_match('~(/[^ ]*/fastchart\.so)(?:\s|$)~', $mapping, $match)) {
		$module = $match[1];
		break;
	}
}
if ($module === null) die("loaded module path not found\n");
$childCode = <<<'PHP'
umask(0022);
$chart = (new FastChart\LineChart(640, 360))
	->setSeries(array_fill(0, 1024, 50.0));
try {
	echo $chart->renderToFile($argv[1]), "\n";
} catch (Throwable $e) {
	fwrite(STDERR, get_class($e) . ': ' . $e->getMessage() . "\n");
	exit(3);
}
PHP;
$command = [
	'strace', '-qq', '-e', 'trace=linkat,renameat2',
	'-e', 'inject=renameat2:error=EOPNOTSUPP:when=1',
	'-e', 'inject=linkat:delay_enter=2s:when=1',
	'--', 'env', 'ASAN_OPTIONS=detect_leaks=0',
	PHP_BINARY, '-n', '-d', 'extension=' . $module,
	'-r', $childCode, $target,
];
$process = proc_open($command, [
	['pipe', 'r'],
	['pipe', 'w'],
	['pipe', 'w'],
], $pipes);
fclose($pipes[0]);

$temp = null;
$modeFinal = false;
$deadline = microtime(true) + 30.0;
while (microtime(true) < $deadline) {
	$matches = glob($target . '.fctmp-*');
	if ($matches) {
		$temp = $matches[0];
		clearstatcache(true, $temp);
		if ((fileperms($temp) & 07777) === 0644) {
			$modeFinal = true;
			break;
		}
	}
	$status = proc_get_status($process);
	if (!$status['running']) break;
	usleep(100);
}

$held = null;
$substituted = false;
if ($temp !== null && $modeFinal) {
	usleep(100_000);
	$held = $temp . '.held';
	$substituted = rename($temp, $held) && symlink($victim, $temp);
}
$stdout = stream_get_contents($pipes[1]);
$stderr = stream_get_contents($pipes[2]);
fclose($pipes[1]);
fclose($pipes[2]);
$exit = proc_close($process);

clearstatcache(true, $target);
clearstatcache(true, $victim);
echo 'new-target race synchronized: ', $substituted ? "yes\n" : "NO\n";
echo 'render succeeded: ',
	$exit === 0 && trim($stdout) !== '' ? "yes\n" : "NO\n";
echo 'target is rendered output: ',
	is_file($target) && str_contains(file_get_contents($target), '<svg')
		? "yes\n" : "NO\n";
echo 'victim preserved: ',
	(fileperms($victim) & 07777) === 0600
	&& file_get_contents($victim) === 'secret'
		? "yes\n" : "NO\n";

foreach ([$temp, $held, $target, $victim] as $path) {
	if ($path !== null && (is_file($path) || is_link($path))) unlink($path);
}
rmdir($dir);

?>
--EXPECT--
new-target race synchronized: yes
render succeeded: yes
target is rendered output: yes
victim preserved: yes
