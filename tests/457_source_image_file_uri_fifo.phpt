--TEST--
Source-image loader rejects a writerless FIFO addressed with file:// without blocking
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

$fifo = __DIR__ . '/457_file_uri.fifo';
@unlink($fifo);
shell_exec('mkfifo ' . escapeshellarg($fifo));
if (!file_exists($fifo)) die('mkfifo failed');

require __DIR__ . '/_resource_process.inc';
$code = '$c = (new FastChart\LineChart(120, 80))'
    . '->setBackgroundImage(' . var_export('file://' . $fifo, true) . ')'
    . '->setSeries([1, 2, 3]); $c->renderSvg();';
$descriptors = [0 => ['pipe', 'r'], 1 => ['pipe', 'w'], 2 => ['pipe', 'w']];
$pipes = [];
$child = proc_open(
    fc_resource_child_command($code),
    $descriptors, $pipes, null, null, ['bypass_shell' => true]
);
if (!is_resource($child)) die("skip child process unavailable\n");

echo 'file-uri-fifo: ' . fc_resource_child_result($child, $pipes) . "\n";
@unlink($fifo);

?>
--EXPECT--
file-uri-fifo: completed
