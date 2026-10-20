--TEST--
Waterfall: a fatal error inside layout leaves the chart owning only its own labels
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
memory_limit=8M
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';
/* The first text measurement inside layout loads the whole font file into
 * request memory; leave less room than that so memory_limit bails out of
 * the layout call. */
$font = fc_pick_font();
$room = min(300000, intdiv(filesize($font), 2));
$chart = new FastChart\Waterfall(400, 300);
$bars = [];
for ($i = 0; $i < 8; $i++) {
    $bars[] = ['label' => "bar$i", 'value' => 10 + $i];
}
$chart->setBars($bars);
$expected = array_map(fn($i) => "bar$i", range(0, 7));
$chart->setFontPath($font)->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);

register_shutdown_function(function () use ($chart, $expected) {
    $GLOBALS['pad'] = null;
    /* PHP 8.1 and 8.2 reserve the whole 16 MiB stream cap when the font
     * is read, which the 8M limit that forced the bailout cannot hold. */
    ini_set('memory_limit', '64M');
    echo "shutdown\n";
    /* Replacing the category labels must free only arrays the chart owns,
     * and the chart must accept setters again after the aborted render. */
    $chart->setCategoryLabels(['q']);
    $svg = $chart->renderSvg();
    preg_match_all('#<text[^>]*>([^<]*)</text>#', $svg, $m);
    $missing = array_diff($expected, $m[1]);
    echo $missing === [] ? "labels intact\n"
        : "labels damaged: " . implode(',', $missing) . "\n";
});

$GLOBALS['pad'] = [];
while (memory_get_usage() + $room + 16384 < 8 * 1024 * 1024) {
    $GLOBALS['pad'][] = str_repeat('x', 16000);
}
$chart->renderSvg();
echo "render unexpectedly fit in memory\n";
?>
--EXPECTF--
Fatal error: Allowed memory size of 8388608 bytes exhausted%s(tried to allocate %d bytes) in %s on line %d
shutdown
labels intact
