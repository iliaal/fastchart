--TEST--
svgToPng() refuses a raster frame larger than memory_limit without killing the process
--DESCRIPTION--
A 4096x4096 frame is 64 MiB, four times this memory_limit. On engines that
queue memory-limit errors (ZEND_HAS_OOM) the infallible allocation crossed
the hard limit and the engine _exit()ed the process.
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (getenv('USE_ZEND_ALLOC') === '0' && getenv('USE_TRACKED_ALLOC') !== '1') {
    die("skip memory_limit is not enforced without ZendMM or tracked allocation\n");
}
?>
--INI--
memory_limit=16M
--FILE--
<?php
$png = FastChart\Chart::svgToPng('<svg xmlns="http://www.w3.org/2000/svg" '
    . 'width="4096" height="4096"><rect width="10" height="10"/></svg>');
echo "returned ", strlen($png), " bytes\n";
?>
--EXPECTF--
Fatal error: Allowed memory size of %d bytes exhausted%s(tried to allocate %d bytes) in %s on line %d%A
