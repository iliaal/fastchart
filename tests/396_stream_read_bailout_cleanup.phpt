--TEST--
Source-image stream closes when memory exhaustion bails out during read
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (PHP_OS_FAMILY !== 'Linux' || !is_dir('/proc/self/fd')) {
    echo "skip: requires Linux /proc/self/fd\n";
}
?>
--FILE--
<?php
$path = tempnam(sys_get_temp_dir(), 'fastchart-bailout-');
$fp = fopen($path, 'wb');
fwrite($fp, "\x89PNG\r\n\x1a\n");
$chunk = str_repeat('x', 8192);
for ($i = 0; $i < 512; $i++) {
    fwrite($fp, $chunk);
}
fclose($fp);
unset($chunk);

register_shutdown_function(function () use ($path): void {
    $open = false;
    foreach (glob('/proc/self/fd/*') as $fd) {
        if (@readlink($fd) === $path) {
            $open = true;
            break;
        }
    }
    echo $open ? "source stream OPEN\n" : "source stream closed\n";
    @unlink($path);
});

$limit = memory_get_usage(true) + 1024 * 1024;
ini_set('memory_limit', (string)$limit);
(new FastChart\LineChart(300, 180))
    ->setSeries([1, 2, 3])
    ->setBackgroundImage($path)
    ->renderSvg();
?>
--EXPECTF--
Fatal error: Allowed memory size of %d bytes exhausted%a(tried to allocate %d bytes) in %s on line %d
source stream closed
