--TEST--
renderPng() stops writing encoded output once memory_limit is exceeded
--DESCRIPTION--
On PHP builds that defer memory-limit errors (ZEND_HAS_OOM), the encoder
kept appending to the output buffer after it crossed memory_limit. An
uncompressed 4096x4096 PNG is about 50 MB on top of the 64 MiB frame, so
the buffer reached the hard limit (twice memory_limit) and the engine
_exit()ed the process.
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
memory_limit=80M
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';
$png = (new FastChart\BarChart(4096, 4096))
    ->setFontPath(fc_pick_font())
    ->setSeries([1, 2, 3])
    ->setPngCompressionLevel(0)
    ->renderPng();
echo "returned ", strlen($png), " bytes\n";
?>
--EXPECTF--
Fatal error: Allowed memory size of %d bytes exhausted%s(tried to allocate %d bytes) in %s on line %d%A
