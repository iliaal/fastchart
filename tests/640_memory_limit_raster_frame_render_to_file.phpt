--TEST--
renderToFile() leaves no temporary file when the raster frame exceeds memory_limit
--DESCRIPTION--
On PHP builds that defer memory-limit errors (ZEND_HAS_OOM) the 256 MiB frame
allocation crossed the hard limit and the engine _exit()ed the process,
stranding the atomic-write temporary file next to the target.
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die("skip no system font available\n");
if (getenv('USE_ZEND_ALLOC') === '0' && getenv('USE_TRACKED_ALLOC') !== '1') {
    die("skip memory_limit is not enforced without ZendMM or tracked allocation\n");
}
require __DIR__ . '/_bailout_shutdown.inc';
if (!fc_oom_runs_shutdown()) {
    die("skip memory_limit exhaustion exits without running shutdown functions\n");
}
?>
--INI--
memory_limit=128M
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';
$path = sys_get_temp_dir() . '/fastchart_640_' . getmypid() . '.png';
register_shutdown_function(function () use ($path) {
    $left = glob($path . '*');
    echo "\nleftover files: ", count($left), "\n";
    foreach ($left as $f) {
        unlink($f);
    }
});
(new FastChart\BarChart(8192, 8192))
    ->setFontPath(fc_pick_font())
    ->setSeries([1, 2, 3])
    ->renderToFile($path);
echo "returned\n";
?>
--EXPECTF--
Fatal error: Allowed memory size of %d bytes exhausted%s(tried to allocate %d bytes) in %s on line %d
%A
leftover files: 0
