--TEST--
A render stopped inside an encoder releases the output it had written
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (!function_exists('exec') || getenv('TEST_PHP_ARGS') === false) {
    echo "skip needs exec() and the run-tests TEST_PHP_ARGS\n";
}
if (@ini_set('memory_limit', '24M') === false) {
    echo "skip memory_limit is not settable here\n";
}
?>
--FILE--
<?php

/* The render paths own two things a bailout inside an encoder strands:
 * the RGBA frame and the bytes the encoder already streamed into the
 * sink. The frame was always released; the sink buffer was not, so a
 * buffer-API render (renderPng/renderJpeg/renderWebp) kept its partial
 * output alive for the rest of the request.
 *
 * A memory_limit bail-out is what makes that measurable: unlike the
 * execution-timeout bail-out it still runs registered shutdown
 * functions, so the child can report what is still held once the
 * bailout has unwound. The limit is set so the trip lands inside the
 * encode -- the frame fits, the output does not.
 *
 * The asserted number is a bound, not a count: what is still live at
 * shutdown is the chart, its series and whatever the allocator holds in
 * its own bookkeeping, all of which move with the chart and the build.
 * What the bound rules out is the order of magnitude of a stranded
 * output buffer (tens of MiB here), which is what the missing release
 * leaves behind. The file-path half is covered separately by
 * tests/478_encode_timeout_leaves_no_file.phpt. */

$dir = sys_get_temp_dir() . '/fastchart-bailout-' . getmypid();
@mkdir($dir, 0777, true);
$child = $dir . '/child.php';
file_put_contents($child, <<<'CHILD'
<?php
/* Child of tests/477: trip memory_limit inside the PNG encode and
 * report what the request still holds when the bailout unwinds. */
$points = [];
for ($i = 0; $i < 512; $i++) { $points[] = (($i * 7919) % 1000) / 1000; }
$chart = (new FastChart\LineChart(2048, 2048))
    ->setSeries([['name' => 's', 'data' => $points]]);
/* Stored (uncompressed) PNG: the output stream is large enough that the
 * limit is crossed part-way through it, not before it starts. */
$chart->setPngCompressionLevel(0);

register_shutdown_function(static function (): void {
    printf("retained %d\n", memory_get_usage(false));
});

ini_set('memory_limit', '24M');
$chart->renderPng();
echo "not reached\n";
CHILD
);

$command = escapeshellarg(PHP_BINARY) . ' ' . getenv('TEST_PHP_ARGS')
    . ' -d memory_limit=-1 ' . escapeshellarg($child) . ' 2>&1';
$output = [];
exec($command, $output, $status);
@unlink($child);
@rmdir($dir);
$text = implode("\n", $output);

/* The limit has to have tripped inside the render, or the run proves
 * nothing: an encoder that returned would print its line. */
if (!str_contains($text, 'retained ')) {
    echo "child retained nothing to report\n";
    var_dump($text);
    return;
}
var_dump(str_contains($text, 'not reached'));
preg_match('/retained (\d+)/', $text, $m);
$retained = (int) $m[1];
var_dump($retained < 1048576);

?>
--CLEAN--
<?php
$dir = sys_get_temp_dir() . '/fastchart-bailout-' . getmypid();
foreach ((array) @scandir($dir) as $f) {
    if ($f !== '.' && $f !== '..') { @unlink($dir . '/' . $f); }
}
@rmdir($dir);
?>
--EXPECT--
bool(false)
bool(true)
