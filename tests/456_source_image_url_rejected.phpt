--TEST--
Source-image loader does not perform network I/O for URL wrappers
--EXTENSIONS--
fastchart
--INI--
allow_url_fopen=1
--SKIPIF--
<?php
if (!function_exists('proc_open') || !function_exists('stream_socket_server')) {
    die("skip process and socket support required\n");
}
?>
--FILE--
<?php

$server = stream_socket_server('tcp://127.0.0.1:0', $errno, $errstr);
if ($server === false) die("skip local listener unavailable: $errstr\n");
$address = stream_socket_get_name($server, false);
$port = (int) substr(strrchr($address, ':'), 1);

$childCode = '$s = stream_socket_server("tcp://127.0.0.1:' . $port . '", $e, $m);'
    . '$c = stream_socket_accept($s, 5);'
    . 'if ($c) fwrite($c, base64_decode("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII="));';
$descriptors = [0 => ['pipe', 'r'], 1 => ['pipe', 'w'], 2 => ['pipe', 'w']];
$pipes = [];
$child = proc_open([PHP_BINARY, '-r', $childCode], $descriptors, $pipes);
if (!is_resource($child)) die("skip child process unavailable\n");

$svg = (new FastChart\LineChart(120, 80))
    ->setBackgroundImage("http://127.0.0.1:$port/image.png")
    ->setSeries([1, 2, 3])
    ->renderSvg();
echo str_contains($svg, '<image ') ? "network-image\n" : "network-rejected\n";

foreach ($pipes as $pipe) fclose($pipe);
proc_terminate($child);
proc_close($child);
fclose($server);

?>
--EXPECT--
network-rejected
