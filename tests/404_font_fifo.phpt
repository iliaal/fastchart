--TEST--
setFontPath rejects a writerless FIFO at render time without blocking
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (!function_exists('shell_exec') || shell_exec('command -v mkfifo') === null) {
    echo "skip mkfifo not available\n";
}
?>
--FILE--
<?php

$fifo = __DIR__ . '/404_font.fifo';
@unlink($fifo);
shell_exec('mkfifo ' . escapeshellarg($fifo));
if (!file_exists($fifo)) die('mkfifo failed');

$svg = (new FastChart\LineChart(200, 150))
    ->setSeries([1, 2, 3])
    ->setTitle('FIFO font')
    ->setFontPath($fifo)
    ->renderSvg();

echo str_contains($svg, '<svg') ? "rendered\n" : "failed\n";
@unlink($fifo);
?>
--CLEAN--
<?php
@unlink(__DIR__ . '/404_font.fifo');
?>
--EXPECT--
rendered
