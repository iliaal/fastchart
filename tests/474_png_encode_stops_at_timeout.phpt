--TEST--
PNG encoding stops at the deadline instead of running the encode out
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

/* The JPEG scanline loop and the un-premultiply pass poll
 * EG(timed_out) every 64 rows; the PNG row loop did not, so a large
 * canvas kept deflating to the last row after the deadline -- the same
 * output, seconds of worker CPU later.
 *
 * Where the render stops is not observable from inside the timing run:
 * a render that runs to completion and one that is stopped both end in
 * the same timeout Error, raised at the next VM interrupt. So the
 * render runs in a child process and the parent compares how much of it
 * was left. The child arms a budget expiring a fraction of a second
 * into the level-9 deflate, prints the wall clock at the start of that
 * render, and dies on the deadline. */

$child = __DIR__ . '/474_png_timeout_child.php';
file_put_contents($child, <<<'CHILD'
<?php
/* Child of tests/474: measure the render stages, then arm a CPU budget
 * that expires inside the PNG encode and render until it is stopped. */
$points = [];
for ($i = 0; $i < 200; $i++) { $points[] = (($i * 7919) % 1000) / 1000; }
$chart = (new FastChart\LineChart(2048, 2048))
    ->setSeries([['name' => 's', 'data' => $points]]);

/* Brings the process back into the VM after the encoder bails out, so
 * the timeout Error is reported rather than the request ending
 * silently. */
register_shutdown_function(static function (): void {});

/* Level 0 skips deflate, so this render measures what the encoder adds
 * on top of the rasterize stage; level 9 is the encode under test. */
$chart->setPngCompressionLevel(0);
$t = hrtime(true);
$png = $chart->renderPng();
$raster = (hrtime(true) - $t) / 1e9;
$chart->setPngCompressionLevel(9);
$t = hrtime(true);
$png = $chart->renderPng();
$full = (hrtime(true) - $t) / 1e9;

/* The deadline lands $aim into the encode: past the rasterize stage the
 * pre-existing poll already covers, inside the deflate that only the
 * row poll can end. */
$aim = 0.2;
$budget = $raster + $aim;
$limit = (int) ceil($raster + $budget) + 1;
fwrite(STDERR, sprintf("marks %d %.4f %.4f %.4f\n",
    $limit, hrtime(true) / 1e9, $raster, $full));

ini_set('max_execution_time', (string) $limit);
$stop = hrtime(true) / 1e9 + ($limit - $budget);
while (hrtime(true) / 1e9 < $stop) { /* spend the armed budget */ }

$chart->renderPng();
echo "not reached\n";
CHILD
);

$command = escapeshellarg(PHP_BINARY) . ' ' . getenv('TEST_PHP_ARGS')
    . ' -d memory_limit=512M ' . escapeshellarg($child) . ' 2>&1';
/* hrtime() is CLOCK_MONOTONIC in both processes, so the parent's
 * start plus the measured duration is the child's exit on the same
 * clock the child stamped its marks with. */
$startedAt = hrtime(true) / 1e9;
$output = [];
exec($command, $output, $status);
$elapsed = hrtime(true) / 1e9 - $startedAt;
@unlink($child);

$text = implode("\n", $output);
if (!preg_match('/marks (\d+) ([0-9.]+) ([0-9.]+) ([0-9.]+)/', $text, $m)) {
    /* No marks: the child never reached the armed render. */
    echo "child produced no marks\n";
    return;
}
$limit = (float) $m[1];
$renderStart = (float) $m[2];
$raster = (float) $m[3];
$full = (float) $m[4];
/* Marks are printed immediately before the timer is armed, so the
 * deadline is $limit seconds after that mark: the burn leaves the
 * render $raster + $aim seconds of the $limit. */
$deadline = $renderStart + $limit;


/* The deadline stopped the child, and the render never returned. */
var_dump(str_contains($text, 'Maximum execution time'));
var_dump(!str_contains($text, 'not reached'));

/* A stopped render overruns the deadline by the aim point, one 64-row
 * poll granularity and the shutdown; an unpolled encode overruns it by
 * the whole level-9 deflate. When the deflate is too small to tell the
 * two apart, the slack is half of it. */
$encode = $full - $raster;
$slack = $encode > 0.8 ? 0.4 : $encode / 2;
var_dump($startedAt + $elapsed < $deadline + $slack);

?>
--CLEAN--
<?php
@unlink(__DIR__ . '/474_png_timeout_child.php');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
