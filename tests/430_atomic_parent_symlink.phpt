--TEST--
renderToFile pins a symlinked parent directory during atomic output
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

$root = sys_get_temp_dir() . '/fastchart-parent-race-'
	. bin2hex(random_bytes(6));
$a = $root . '/a';
$b = $root . '/b';
$link = $root . '/current';
mkdir($root, 0700);
mkdir($a, 0700);
mkdir($b, 0700);
symlink($a, $link);
$target = $link . '/out.png';
$victim = $b . '/out.png';
file_put_contents($victim, 'secret');

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
$process = proc_open($command, [
	['pipe', 'r'],
	['pipe', 'w'],
	['pipe', 'w'],
], $pipes);
fclose($pipes[0]);

$temp = null;
$deadline = microtime(true) + 30.0;
while (microtime(true) < $deadline) {
	$matches = glob($a . '/out.png.fctmp-*');
	if ($matches) {
		$temp = $matches[0];
		break;
	}
	$status = proc_get_status($process);
	if (!$status['running']) break;
	usleep(100);
}
$swapped = $temp !== null && unlink($link) && symlink($b, $link);
$stdout = stream_get_contents($pipes[1]);
$stderr = stream_get_contents($pipes[2]);
fclose($pipes[1]);
fclose($pipes[2]);
$exit = proc_close($process);

echo 'parent race synchronized: ', $swapped ? "yes\n" : "NO\n";
$bytes = trim($stdout);
echo 'render completed: ',
	$exit === 0 && $stderr === '' && $bytes !== ''
	&& strspn($bytes, '0123456789') === strlen($bytes)
		? "yes\n" : "NO\n";
echo 'opened directory received output: ',
	is_file($a . '/out.png')
	&& str_starts_with(file_get_contents($a . '/out.png'), "\x89PNG")
		? "yes\n" : "NO\n";
echo 'other directory preserved: ',
	file_get_contents($victim) === 'secret' ? "yes\n" : "NO\n";

foreach (glob($a . '/*') as $path) unlink($path);
unlink($victim);
unlink($link);
rmdir($a);
rmdir($b);
rmdir($root);

?>
--EXPECT--
parent race synchronized: yes
render completed: yes
opened directory received output: yes
other directory preserved: yes
