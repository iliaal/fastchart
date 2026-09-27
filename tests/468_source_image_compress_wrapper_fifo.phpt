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

require __DIR__ . '/_resource_process.inc';
$code = '$c = (new FastChart\LineChart(120, 80))'
    . '->setBackgroundImage(' . var_export('compress.zlib://' . $fifo, true) . ')'
    . '->setSeries([1, 2, 3]); $c->renderSvg();';
$descriptors = [0 => ['pipe', 'r'], 1 => ['pipe', 'w'], 2 => ['pipe', 'w']];
$pipes = [];
$child = proc_open(
    fc_resource_child_command($code),
    $descriptors, $pipes, null, null, ['bypass_shell' => true]
);
if (!is_resource($child)) die("skip child process unavailable\n");

echo 'compress-wrapper-fifo: ' . fc_resource_child_result($child, $pipes) . "\n";
@unlink($fifo);

?>
--EXPECT--
compress-wrapper-fifo: completed
