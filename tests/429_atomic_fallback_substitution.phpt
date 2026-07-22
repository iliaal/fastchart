--TEST--
renderToFile fallback preserves concurrent entries after staging substitution
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (PHP_OS_FAMILY !== 'Linux') die('skip Linux strace fault injection');
if (!function_exists('proc_open')) die('skip proc_open unavailable');
if (!is_readable('/proc/self/maps')) die('skip loaded module path unavailable');
$probe = proc_open([
	'strace', '-qq', '-e', 'trace=renameat',
	'-e', 'inject=renameat:delay_enter=1ms:when=1',
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

$dir = sys_get_temp_dir() . '/fastchart-fallback-race-'
	. bin2hex(random_bytes(6));
mkdir($dir, 0700);
$target = $dir . '/out.svg';
$victim = $dir . '/victim';
file_put_contents($target, 'old');
chmod($target, 0777);
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
	'strace', '-qq', '-e', 'trace=renameat,renameat2',
	'-e', 'inject=renameat2:error=EOPNOTSUPP:when=1',
	'-e', 'inject=renameat:delay_enter=2s:when=1',
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

$commit = null;
$backup = null;
$deadline = microtime(true) + 30.0;
while (microtime(true) < $deadline) {
	$temps = glob($target . '.fctmp-*');
	foreach ($temps as $candidate) {
		if (str_ends_with($candidate, '.old')) {
			$backup = $candidate;
		} elseif (str_ends_with($candidate, '.commit')) {
			$commit = $candidate;
		}
	}
	if ($commit !== null && $backup !== null) break;
	$status = proc_get_status($process);
	if (!$status['running']) break;
	usleep(100);
}

$held = null;
$substituted = false;
if ($commit !== null && $backup !== null) {
	$held = $commit . '.held';
	$substituted = rename($commit, $held) && symlink($victim, $commit);
}
$stdout = stream_get_contents($pipes[1]);
$stderr = stream_get_contents($pipes[2]);
fclose($pipes[1]);
fclose($pipes[2]);
$exit = proc_close($process);

clearstatcache(true, $target);
clearstatcache(true, $victim);
echo 'fallback race synchronized: ', $substituted ? "yes\n" : "NO\n";
echo 'render rejected: ',
	$exit === 3
	&& str_contains($stderr,
		'Error: FastChart\\Chart::renderToFile() finalized file changed')
		? "yes\n" : "NO\n";
echo 'concurrent entry preserved: ',
	is_link($target) && readlink($target) === $victim
		? "yes\n" : "NO\n";
echo 'prior target recoverable: ',
	is_file($backup) && file_get_contents($backup) === 'old'
		? "yes\n" : "NO\n";
echo 'victim preserved: ',
	(fileperms($victim) & 07777) === 0600
	&& file_get_contents($victim) === 'secret'
		? "yes\n" : "NO\n";

foreach ([$commit, $held, $backup, $target, $victim] as $path) {
	if ($path !== null && (is_file($path) || is_link($path))) unlink($path);
}
rmdir($dir);

?>
--EXPECT--
fallback race synchronized: yes
render rejected: yes
concurrent entry preserved: yes
prior target recoverable: yes
victim preserved: yes
