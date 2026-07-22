--TEST--
renderToFile rejects a substituted atomic temporary path without following it
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (PHP_OS_FAMILY === 'Windows') die('skip POSIX symlink race');
if (!function_exists('proc_open')) die('skip proc_open unavailable');
$module = realpath(__DIR__ . '/../modules/fastchart.so');
if ($module === false) die('skip module path unavailable');
?>
--INI--
asan.detect_leaks=0
--FILE--
<?php

$dir = sys_get_temp_dir() . '/fastchart-race-' . bin2hex(random_bytes(6));
mkdir($dir, 0700);
$target = $dir . '/out.png';
$victim = $dir . '/victim';
file_put_contents($target, 'old');
chmod($target, 0777);
file_put_contents($victim, 'secret');
chmod($victim, 0600);

$module = realpath(__DIR__ . '/../modules/fastchart.so');
$childCode = <<<'PHP'
$chart = (new FastChart\LineChart(4096, 4096))
	->setSeries(array_fill(0, 2048, 50.0));
try {
	echo $chart->renderToFile($argv[1]), "\n";
} catch (Throwable $e) {
	fwrite(STDERR, get_class($e) . ': ' . $e->getMessage() . "\n");
	exit(3);
}
PHP;
$command = [PHP_BINARY, '-n', '-d', 'extension=' . $module,
	'-r', $childCode, $target];
$pipes = [];
$process = proc_open($command, [
	['pipe', 'r'],
	['pipe', 'w'],
	['pipe', 'w'],
], $pipes);
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
	usleep(100);
}

$held = null;
$substituted = false;
if ($temp !== null) {
	$held = $temp . '.held';
	$substituted = rename($temp, $held) && symlink($victim, $temp);
}
$stdout = stream_get_contents($pipes[1]);
$stderr = stream_get_contents($pipes[2]);
fclose($pipes[1]);
fclose($pipes[2]);
$exit = proc_close($process);

clearstatcache(true, $victim);
echo 'race synchronized: ', $substituted ? "yes\n" : "NO\n";
echo 'render rejected: ',
	$exit === 3
	&& str_contains($stderr, 'Error: FastChart\\Chart::renderToFile()')
	&& (str_contains($stderr, 'could not finalize')
		|| str_contains($stderr, 'finalized file changed'))
		? "yes\n" : "NO\n";
echo 'target preserved: ',
	!is_link($target) && file_get_contents($target) === 'old'
		? "yes\n" : "NO\n";
echo 'victim preserved: ',
	(fileperms($victim) & 07777) === 0600
	&& file_get_contents($victim) === 'secret'
		? "yes\n" : "NO\n";

foreach ([$temp, $held, $target, $victim] as $path) {
	if ($path !== null && (is_file($path) || is_link($path))) unlink($path);
}
$sentinel = $target . '/sentinel';
mkdir($target, 0700);
file_put_contents($sentinel, 'keep');
try {
	(new FastChart\LineChart(64, 64))
		->setSeries([1, 2])
		->renderToFile($target);
	echo "directory rejected: NO\n";
} catch (Error $e) {
	echo 'directory rejected: ',
		str_contains($e->getMessage(), 'cannot replace directory')
			? "yes\n" : "wrong\n";
}
echo 'directory preserved: ',
	is_dir($target)
	&& file_get_contents($sentinel) === 'keep'
	&& glob($target . '.fctmp-*') === []
		? "yes\n" : "NO\n";
unlink($sentinel);
rmdir($target);
rmdir($dir);

?>
--EXPECT--
race synchronized: yes
render rejected: yes
target preserved: yes
victim preserved: yes
directory rejected: yes
directory preserved: yes
