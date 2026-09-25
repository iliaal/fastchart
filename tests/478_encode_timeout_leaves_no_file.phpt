--TEST--
A render stopped by the deadline leaves no output behind
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (!function_exists('exec') || getenv('TEST_PHP_ARGS') === false) {
    echo "skip needs exec() and the run-tests TEST_PHP_ARGS\n";
}
/* The file half of the same ownership rule that
 * tests/477_encoder_bailout_releases_output.phpt covers for the buffer
 * API: renderToFile()'s catch aborts the staging file, so a render
 * stopped mid-encode leaves neither a destination nor a temp behind.
 *
 * Unlike a memory_limit trip, an execution-timeout bail-out does not
 * run registered shutdown functions, so the render is checked from the
 * parent instead: the child must die on the deadline. */
--FILE--
<?php


$dir = sys_get_temp_dir() . '/fastchart-timeout-' . getmypid();
@mkdir($dir, 0777, true);
$child = $dir . '/child.php';
file_put_contents($child, <<<'CHILD'
<?php
/* Child of tests/478: arm a budget that expires inside the encode, then
 * render to a file and to a buffer. */
$points = [];
for ($i = 0; $i < 200; $i++) { $points[] = (($i * 7919) % 1000) / 1000; }
$chart = (new FastChart\LineChart(2048, 2048))
    ->setSeries([['name' => 's', 'data' => $points]]);

/* Brings the process back into the VM after the encoder bails out, so
 * the timeout Error is reported rather than the request ending
 * silently. */
register_shutdown_function(static function (): void {});

ini_set('max_execution_time', '2');
$stop = hrtime(true) / 1e9 + 1.95;
while (hrtime(true) / 1e9 < $stop) { /* spend the armed budget */ }

$chart->renderToFile($argv[1] . '/stopped.png');
echo "file: not reached\n";
$chart->renderPng();
echo "buffer: not reached\n";
CHILD
);

$command = escapeshellarg(PHP_BINARY) . ' ' . getenv('TEST_PHP_ARGS')
    . ' -d memory_limit=512M ' . escapeshellarg($child) . ' '
    . escapeshellarg($dir) . ' 2>&1';
$output = [];
exec($command, $output, $status);
@unlink($child);
$text = implode("\n", $output);

/* The deadline stopped the first render, so the buffer render never ran. */
var_dump(str_contains($text, 'Maximum execution time'));
var_dump(!str_contains($text, 'not reached'));

/* No destination, and no staging temp left in the directory. */
$left = array_values(array_diff(scandir($dir), ['.', '..', 'child.php']));
var_dump($left);

?>
--CLEAN--
<?php
$dir = sys_get_temp_dir() . '/fastchart-timeout-' . getmypid();
foreach ((array) @scandir($dir) as $f) {
    if ($f !== '.' && $f !== '..') { @unlink($dir . '/' . $f); }
}
@rmdir($dir);
?>
--EXPECT--
bool(true)
bool(true)
array(0) {
}
