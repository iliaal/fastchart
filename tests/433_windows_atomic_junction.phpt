--TEST--
Windows renderToFile pins junction parents and revalidates open_basedir
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (PHP_OS_FAMILY !== 'Windows') die('skip Windows junction test');
if (!function_exists('proc_open')) die('skip proc_open unavailable');
$args = getenv('TEST_PHP_ARGS') ?: '';
if (!preg_match('~(?:^|\\s)-d\\s+extension=([^\\s]+php_fastchart\\.dll)~i',
	$args, $match) || !is_file($match[1])) {
	die('skip fastchart DLL path unavailable');
}
$root = sys_get_temp_dir() . '/fc-junction-probe-' . bin2hex(random_bytes(4));
$target = $root . '/target';
$link = $root . '/link';
mkdir($root, 0700);
mkdir($target, 0700);
$process = proc_open(
	['cmd.exe', '/d', '/c', 'mklink', '/J', $link, $target],
	[['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']],
	$pipes
);
if (!is_resource($process)) die('skip cmd unavailable');
foreach ($pipes as $pipe) fclose($pipe);
$exit = proc_close($process);
if ($exit === 0) rmdir($link);
rmdir($target);
rmdir($root);
if ($exit !== 0) die('skip directory junctions unavailable');
?>
--FILE--
<?php

function make_junction(string $link, string $target): bool
{
	$process = proc_open(
		['cmd.exe', '/d', '/c', 'mklink', '/J', $link, $target],
		[['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']],
		$pipes
	);
	if (!is_resource($process)) return false;
	foreach ($pipes as $pipe) fclose($pipe);
	return proc_close($process) === 0;
}

$root = sys_get_temp_dir() . '/fc-junction-' . bin2hex(random_bytes(6));
$source = $root . '/source';
$redirect = $root . '/redirect';
$allowed = $root . '/allowed';
$inside = $allowed . '/inside';
$outside = $root . '/outside';
$link = $root . '/link';
foreach ([$root, $source, $redirect, $allowed, $inside, $outside] as $dir) {
	if (!is_dir($dir)) mkdir($dir, 0700);
}
file_put_contents($redirect . '/victim', 'preserve');
make_junction($link, $source);

$args = getenv('TEST_PHP_ARGS') ?: '';
preg_match('~(?:^|\\s)-d\\s+extension=([^\\s]+php_fastchart\\.dll)~i',
	$args, $moduleMatch);
$module = $moduleMatch[1];
$childCode = <<<'PHP'
try {
	$chart = (new FastChart\LineChart(4096, 4096))
		->setSeries(array_fill(0, 2048, 50.0));
	echo $chart->renderToFile($argv[1]), "\n";
} catch (Throwable $e) {
	fwrite(STDERR, get_class($e) . ': ' . $e->getMessage() . "\n");
	exit(3);
}
PHP;
$target = $link . '/out.png';
$process = proc_open([
	PHP_BINARY, '-n', '-d', 'extension=' . $module,
	'-r', $childCode, $target,
], [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $pipes);
fclose($pipes[0]);

$tempSeen = false;
$deadline = microtime(true) + 30.0;
while (microtime(true) < $deadline) {
	if (glob($source . '/out.png.fctmp-*')) {
		$tempSeen = true;
		break;
	}
	$status = proc_get_status($process);
	if (!$status['running']) break;
	usleep(1000);
}
$switched = $tempSeen && rmdir($link) && make_junction($link, $redirect);
$stdout = stream_get_contents($pipes[1]);
$stderr = stream_get_contents($pipes[2]);
fclose($pipes[1]);
fclose($pipes[2]);
$exit = proc_close($process);

echo 'junction race synchronized: ', $switched ? "yes\n" : "NO\n";
echo 'render succeeded: ',
	$exit === 0 && trim($stdout) !== '' ? "yes\n" : "NO\n";
echo 'output stayed in pinned parent: ',
	is_file($source . '/out.png') && !file_exists($redirect . '/out.png')
		? "yes\n" : "NO\n";
echo 'redirect victim preserved: ',
	file_get_contents($redirect . '/victim') === 'preserve'
		? "yes\n" : "NO\n";

rmdir($link);
make_junction($allowed . '/escape', $outside);
$outsideTarget = $allowed . '/escape/out.svg';
$openBasedirCode = <<<'PHP'
echo 'extension loaded: ', extension_loaded('fastchart') ? "yes\n" : "NO\n";
try {
	(new FastChart\LineChart(160, 100))
		->setSeries([1, 2])
		->renderToFile($argv[1]);
	echo "accepted\n";
} catch (Throwable $e) {
	echo get_class($e), ': ', $e->getMessage(), "\n";
}
PHP;
$check = proc_open([
	PHP_BINARY, '-n', '-d', 'extension=' . $module,
	'-d', 'open_basedir=' . $allowed,
	'-r', $openBasedirCode, $outsideTarget,
], [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $checkPipes);
fclose($checkPipes[0]);
$checkOut = stream_get_contents($checkPipes[1]);
$checkErr = stream_get_contents($checkPipes[2]);
fclose($checkPipes[1]);
fclose($checkPipes[2]);
$checkExit = proc_close($check);
echo 'outside junction rejected: ',
	$checkExit === 0
	&& str_contains($checkOut, "extension loaded: yes\n")
	&& str_contains($checkOut, 'Error: ')
	&& str_contains($checkOut, 'open_basedir')
	&& !file_exists($outside . '/out.svg') ? "yes\n" : "NO\n";

rmdir($allowed . '/escape');
$race = $allowed . '/race';
$ready = $allowed . '/ready';
make_junction($race, $inside);
$raceCode = <<<'PHP'
$chart = (new FastChart\LineChart(4096, 4096))
	->setSeries(array_fill(0, 2048, 50.0));
$text = str_repeat('RaceWindow', 50);
for ($i = 0; $i < 120; $i++) {
	$chart->addTextAnnotation($text, 20, 20 + ($i % 100));
}
file_put_contents($argv[2], 'ready');
try {
	$chart->renderToFile($argv[1]);
	echo "accepted\n";
} catch (Throwable $e) {
	echo get_class($e), ': ', $e->getMessage(), "\n";
}
PHP;
$raceProcess = proc_open([
	PHP_BINARY, '-n', '-d', 'extension=' . $module,
	'-d', 'open_basedir=' . $allowed,
	'-r', $raceCode, $race . '/out.svg', $ready,
], [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $racePipes);
fclose($racePipes[0]);
$readySeen = false;
$raceDeadline = microtime(true) + 30.0;
while (microtime(true) < $raceDeadline) {
	if (is_file($ready)) {
		$readySeen = true;
		break;
	}
	$status = proc_get_status($raceProcess);
	if (!$status['running']) break;
	usleep(1000);
}
usleep(5000);
$raceSwitched = $readySeen && rmdir($race)
	&& make_junction($race, $outside);
$raceOut = stream_get_contents($racePipes[1]);
$raceErr = stream_get_contents($racePipes[2]);
fclose($racePipes[1]);
fclose($racePipes[2]);
$raceExit = proc_close($raceProcess);
echo 'open_basedir race synchronized: ', $raceSwitched ? "yes\n" : "NO\n";
echo 'resolved parent rejected: ',
	$raceExit === 0
	&& str_contains($raceOut, 'Error: ')
	&& str_contains($raceOut, 'parent directory is outside open_basedir')
	&& !file_exists($inside . '/out.svg')
	&& !file_exists($outside . '/out.svg') ? "yes\n" : "NO\n";

rmdir($race);
@unlink($ready);
$movable = $allowed . '/movable';
$moved = $outside . '/moved';
mkdir($movable, 0700);
$moveTarget = $movable . '/out.png';
$moveProcess = proc_open([
	PHP_BINARY, '-n', '-d', 'extension=' . $module,
	'-d', 'open_basedir=' . $allowed,
	'-r', $childCode, $moveTarget,
], [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $movePipes);
fclose($movePipes[0]);
$moveTempSeen = false;
$moveDeadline = microtime(true) + 30.0;
while (microtime(true) < $moveDeadline) {
	if (glob($movable . '/out.png.fctmp-*')) {
		$moveTempSeen = true;
		break;
	}
	$status = proc_get_status($moveProcess);
	if (!$status['running']) break;
	usleep(1000);
}
$relocationBlocked = $moveTempSeen && !@rename($movable, $moved);
$moveOut = stream_get_contents($movePipes[1]);
$moveErr = stream_get_contents($movePipes[2]);
fclose($movePipes[1]);
fclose($movePipes[2]);
$moveExit = proc_close($moveProcess);
echo 'parent relocation blocked: ',
	$relocationBlocked ? "yes\n" : "NO\n";
echo 'relocation render succeeded: ',
	$moveExit === 0 && trim($moveOut) !== ''
	&& is_file($moveTarget) && !file_exists($moved . '/out.png')
		? "yes\n" : "NO\n";

@unlink($source . '/out.png');
@unlink($redirect . '/victim');
@unlink($moveTarget);
foreach (glob($source . '/out.png.fctmp-*') as $temp) @unlink($temp);
foreach ([$source, $redirect, $inside, $movable, $allowed, $moved,
	$outside, $root] as $dir) {
	@rmdir($dir);
}

?>
--EXPECT--
junction race synchronized: yes
render succeeded: yes
output stayed in pinned parent: yes
redirect victim preserved: yes
outside junction rejected: yes
open_basedir race synchronized: yes
resolved parent rejected: yes
parent relocation blocked: yes
relocation render succeeded: yes
