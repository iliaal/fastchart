--TEST--
renderSvg() stops emitting glyph paths once memory_limit is exceeded
--DESCRIPTION--
On engines that queue memory-limit errors (ZEND_HAS_OOM), an SVG build that
kept appending after crossing memory_limit reached the hard limit (twice
memory_limit) and the engine _exit()ed the process. The render must stop so
the ordinary memory-limit fatal error is reported instead.
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
memory_limit=4M
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';
$values = [];
for ($i = 0; $i < 2048; $i++) {
    $values[] = 123456.789 + $i;
}
/* About 10 MB of glyph-path value labels from well under 1 MB of input. */
$svg = (new FastChart\LineChart(4000, 3000))
    ->setFontPath(fc_pick_font())
    ->setSeries($values)
    ->setShowValues(true, '%.6f')
    ->renderSvg();
echo "returned ", strlen($svg), " bytes\n";
?>
--EXPECTF--
Fatal error: Allowed memory size of %d bytes exhausted%s(tried to allocate %d bytes) in %s on line %d%A
