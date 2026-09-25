--TEST--
Source-image loader rejects php descriptor wrappers before inherited-pipe reads
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (!function_exists('proc_open')) die("skip process support required\n");
?>
--FILE--
<?php

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
    . '->setBackgroundImage("php://fd/0")'
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
    echo "php-fd-wrapper: timed-out\n";
} else {
    $stderr = stream_get_contents($pipes[2]);
    $exit = proc_close($child);
    echo ($exit === 0 && $stderr === '' ? "php-fd-wrapper: completed\n" : "php-fd-wrapper: failed\n");
}

?>
--EXPECT--
php-fd-wrapper: completed
