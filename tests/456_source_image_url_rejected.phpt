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

require __DIR__ . '/_resource_http.inc';
[$child, $pipes, $url] = fc_resource_http_start();
try {
    $png = fc_resource_http_get($url . '/image.png');
    echo $png === base64_decode('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII=')
        ? "http-control: served\n" : "http-control: failed\n";
    $svg = (new FastChart\LineChart(120, 80))
        ->setBackgroundImage($url . '/image.png')
        ->setSeries([1, 2, 3])
        ->renderSvg();
    echo str_contains($svg, '<image ') ? "network-image\n" : "network-rejected\n";
    echo 'http-requests: ' . fc_resource_http_get($url . '/stop') . "\n";
    echo 'http-child: ' . fc_resource_child_result($child, $pipes) . "\n";
} finally {
    if (is_resource($child)) {
        proc_terminate($child);
        foreach ($pipes as $pipe) if (is_resource($pipe)) fclose($pipe);
        proc_close($child);
    }
}

?>
--EXPECT--
http-control: served
network-rejected
http-requests: 1
http-child: completed
