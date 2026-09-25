--TEST--
Source-image loader rejects compress.zlib FIFO indirection without blocking
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (!function_exists('proc_open') || !function_exists('shell_exec')
    || shell_exec('command -v mkfifo') === null) {
    die("skip process and mkfifo support required\n");
}
?>
--FILE--
<?php

$fifo = __DIR__ . '/468_compress.fifo';
@unlink($fifo);
shell_exec('mkfifo ' . escapeshellarg($fifo));
if (!file_exists($fifo)) die('mkfifo failed');

$args = getenv('TEST_PHP_ARGS') ?: '';
$extension = null;
if (preg_match('/(?:^|\s)-d\s+extension=(\S+)/', $args, $match)) {
    $extension = $match[1];
}
if ($extension === null) {
    if (PHP_OS_FAMILY === 'Windows') die("skip TEST_PHP_ARGS extension path unavailable\n");
    $extension = dirname(__DIR__) . '/modules/fastchart.so';
}
$code = '$c = (new FastChart\LineChart(120, 80))'
    . '->setBackgroundImage(' . var_export('compress.zlib://' . $fifo, true) . ')'
    . '->setSeries([1, 2, 3]); $c->renderSvg();';
$descriptors = [0 => ['pipe', 'r'], 1 => ['pipe', 'w'], 2 => ['pipe', 'w']];
$pipes = [];
$child = proc_open(
    [PHP_BINARY, '-n', '-d', 'extension=' . $extension, '-r', $code],
    $descriptors, $pipes
);
if (!is_resource($child)) die("skip child process unavailable\n");

$done = false;
for ($i = 0; $i < 20; $i++) {
    usleep(100000);
    if (!proc_get_status($child)['running']) {
        $done = true;
        break;
    }
}
if (!$done) {
    proc_terminate($child);
    proc_close($child);
    echo "compress-wrapper-fifo: timed-out\n";
} else {
    $stderr = stream_get_contents($pipes[2]);
    $exit = proc_close($child);
    echo ($exit === 0 && $stderr === '' ? "compress-wrapper-fifo: completed\n" : "compress-wrapper-fifo: failed\n");
}
@unlink($fifo);

?>
--EXPECT--
compress-wrapper-fifo: completed
