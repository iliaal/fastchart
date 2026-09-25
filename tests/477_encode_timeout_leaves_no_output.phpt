--TEST--
A render stopped by the deadline leaves no output behind
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (!function_exists('exec') || getenv('TEST_PHP_ARGS') === false) {
    echo "skip needs exec() and the run-tests TEST_PHP_ARGS\n";
}
?>
--FILE--
<?php

/* The render paths own two things a bailout inside an encoder would
 * strand: the RGBA frame and the bytes already streamed into the sink.
 * Both catches release them -- the file path also aborts its staging
 * file. That last one is the part a user can see, so it is what this
 * pins: a render stopped mid-encode leaves neither a destination nor a
 * staging temp behind.
 *
 * The buffer-API half of the same catch (renderPng()'s smart_str) is not
 * observable from userland: the request ends on the bailout, and the
 * peak resident size was already reached before anything is released.
 * This test covers it as far as userland can -- the child must die on
 * the deadline, not return partial bytes. */

$dir = sys_get_temp_dir() . '/fastchart-timeout-' . getmypid();
@mkdir($dir, 0777, true);
$child = $dir . '/child.php';
file_put_contents($child, <<<'CHILD'
<?php
/* Child of tests/477: arm a budget that expires inside the encode, then
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
