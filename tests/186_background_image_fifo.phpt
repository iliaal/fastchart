--TEST--
setBackgroundImage(): writerless FIFO is rejected without blocking
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

/* open(2) on a FIFO with no writer blocks forever; the non-regular-
 * file check must therefore run BEFORE the open. Pre-fix this test
 * hangs until the run-tests timeout kills it. */

$fifo = __DIR__ . '/186_bg.fifo';
@unlink($fifo);
shell_exec('mkfifo ' . escapeshellarg($fifo));
if (!file_exists($fifo)) die('mkfifo failed');

$c = (new FastChart\LineChart(200, 150))
    ->setSeries([['data' => [1, 2, 3]]])
    ->setBackgroundImage($fifo);

/* The loader falls back to the solid background; the render must
 * complete and produce a valid PNG. */
$png = $c->renderPng();
var_dump(substr($png, 1, 3));

@unlink($fifo);
echo "done\n";

?>
--CLEAN--
<?php
@unlink(__DIR__ . '/186_bg.fifo');
?>
--EXPECT--
string(3) "PNG"
done
