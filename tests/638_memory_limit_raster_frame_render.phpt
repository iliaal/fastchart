--TEST--
renderPng() refuses a raster frame larger than memory_limit without killing the process
--DESCRIPTION--
An 8192x8192 frame is 256 MiB, twice the default memory_limit. On PHP
builds that defer memory-limit errors (ZEND_HAS_OOM) the infallible allocation
crossed the hard limit and the engine _exit()ed the process.
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die("skip no system font available\n");
if (getenv('USE_ZEND_ALLOC') === '0' && getenv('USE_TRACKED_ALLOC') !== '1') {
    die("skip memory_limit is not enforced without ZendMM or tracked allocation\n");
}
?>
--INI--
memory_limit=128M
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';
$png = (new FastChart\BarChart(8192, 8192))
    ->setFontPath(fc_pick_font())
    ->setSeries([1, 2, 3])
    ->renderPng();
echo "returned ", strlen($png), " bytes\n";
?>
--EXPECTF--
Fatal error: Allowed memory size of %d bytes exhausted%s(tried to allocate %d bytes) in %s on line %d%A
