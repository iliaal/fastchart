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

require __DIR__ . '/_resource_process.inc';
$code = '$c = (new FastChart\LineChart(120, 80))'
    . '->setBackgroundImage("php://fd/0")'
    . '->setSeries([1, 2, 3]); $c->renderSvg();';
$descriptors = [0 => ['pipe', 'r'], 1 => ['pipe', 'w'], 2 => ['pipe', 'w']];
$pipes = [];
$child = proc_open(
    fc_resource_child_command($code),
    $descriptors, $pipes, null, null, ['bypass_shell' => true]
);
if (!is_resource($child)) die("skip child process unavailable\n");

echo 'php-fd-wrapper: ' . fc_resource_child_result($child, $pipes) . "\n";

?>
--EXPECT--
php-fd-wrapper: completed
