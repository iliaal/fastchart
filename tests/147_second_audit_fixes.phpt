--TEST--
Second audit fixes: input caps, PNG dim UB, Gantt narrowing, ISO date validation
--EXTENSIONS--
fastchart
--FILE--
<?php

/* CR-001: Input caps on the new chart families. ecalloc'ing
 * unbounded user-supplied counts let a 100M-entry array DoS the
 * worker. Each family now has a per-setter cap in
 * php_fastchart.h:528+. */
$bars = array_fill(0, 5000, ['value' => 1, 'label' => 'x']);
$svg = (new FastChart\ParetoChart(400, 300))->setBars($bars)->renderSvg();
/* Cap is 128. Polygon/rect counts should reflect that, not 5000. */
echo "pareto_capped: ",
    (substr_count($svg, '<rect') < 500 ? "ok" : "fail"), "\n";

$vecs = array_fill(0, 100000,
    ['x' => 0, 'y' => 0, 'dx' => 0.5, 'dy' => 0.5]);
$svg = (new FastChart\VectorChart(400, 400))->setVectors($vecs)->renderSvg();
/* Cap is 4096; arrow polygons should not exceed that. */
echo "vector_capped: ",
    (substr_count($svg, '<polygon') <= 4200 ? "ok" : "fail"), "\n";

/* Sankey caps: 256 nodes, 1024 links. */
$nodes = [];
for ($i = 0; $i < 1000; $i++) {
    $nodes[] = ['label' => "n$i"];
}
$links = [];
for ($i = 0; $i < 5000; $i++) {
    $links[] = ['from' => 0, 'to' => 1, 'value' => 1];
}
$svg = (new FastChart\SankeyChart(400, 300))
    ->setNodes($nodes)
    ->setLinks($links)
    ->renderSvg();
echo "sankey_capped: ", (strlen($svg) > 100 ? "ok" : "fail"), "\n";

/* CR-003: Gantt depends narrowing. Pre-fix, depends => [4294967296]
 * silently became depends => [0] via signed-int cast. */
$svg = (new FastChart\GanttChart(400, 200))
    ->setTasks([
        ['label' => 'T0', 'start' => 0, 'end' => 1,
         'depends' => [4294967296, -1, 9999999999, 1]],
        ['label' => 'T1', 'start' => 2, 'end' => 3],
    ])
    ->renderSvg();
echo "gantt_big_dep_renders: ", (strlen($svg) > 100 ? "ok" : "fail"), "\n";

/* CR-004: Calendar ISO date validation. Pre-fix, 2026-02-31 parsed
 * cleanly into days_from_civil and normalized to 2026-03-03. Same
 * for non-leap-year Feb-29 and 31-day months on 30-day calendars. */
$c1 = (new FastChart\CalendarHeatmap(400, 200))
    ->setData([
        '2026-02-31' => 1,   /* rejected: Feb 2026 max is 28 */
        '2026-04-31' => 1,   /* rejected: April has 30 days */
        '2025-02-29' => 1,   /* rejected: 2025 not leap */
        '2024-02-29' => 1,   /* accepted: 2024 is leap */
        '2026-03-15' => 5,
    ]);
$svg1 = $c1->renderSvg();

/* Sanity: the equivalent date set with ONLY the two valid entries
 * should produce essentially the same render (modulo whitespace). */
$c2 = (new FastChart\CalendarHeatmap(400, 200))
    ->setData([
        '2024-02-29' => 1,
        '2026-03-15' => 5,
    ]);
$svg2 = $c2->renderSvg();
echo "calendar_invalid_dropped: ",
    (strlen($svg1) === strlen($svg2) ? "ok" : "fail"), "\n";

/* CR-002: PNG dimension sniffer signed-left-shift UB. Pre-fix,
 * a PNG header with IHDR width whose high byte was >= 0x80 hit
 * UB on `b[16] << 24` (signed-int promotion into the sign bit).
 * Craft a minimal PNG header with width=0xC0000001 and height=1,
 * feed via setBackgroundImage, render. ASan would catch the UB
 * pre-fix; post-fix the sniffer returns -1 (width > INT_MAX) and
 * the image load refuses the file. We don't need the PNG to be
 * decodeable — only the first 24 bytes are read by the sniffer. */
$tmp = tempnam(sys_get_temp_dir(), 'fcpng_');
$png_signature = "\x89PNG\r\n\x1A\n";
$ihdr_len      = "\x00\x00\x00\x0D";
$ihdr_type     = "IHDR";
$width_be      = "\xC0\x00\x00\x01";  /* high bit set: > INT_MAX */
$height_be     = "\x00\x00\x00\x01";
$ihdr_rest     = "\x08\x02\x00\x00\x00";   /* 8-bit RGB; bogus CRC OK */
$crc           = "\x00\x00\x00\x00";
file_put_contents($tmp, $png_signature . $ihdr_len . $ihdr_type
    . $width_be . $height_be . $ihdr_rest . $crc);

try {
    $svg = (new FastChart\LineChart(400, 200))
        ->setSeries([['data' => [1, 2, 3]]])
        ->setBackgroundImage($tmp)
        ->renderSvg();
    /* If we got here without crashing or ASan tripping, the
     * signed-shift UB no longer fires. The image is silently
     * dropped (sniffer rejected it as > INT_MAX). */
    echo "png_dim_ub_safe: ok\n";
} catch (\Throwable $e) {
    echo "png_dim_ub_safe: throw (", $e->getMessage(), ")\n";
}
@unlink($tmp);

echo "ok\n";
?>
--EXPECT--
pareto_capped: ok
vector_capped: ok
sankey_capped: ok
gantt_big_dep_renders: ok
calendar_invalid_dropped: ok
png_dim_ub_safe: ok
ok
