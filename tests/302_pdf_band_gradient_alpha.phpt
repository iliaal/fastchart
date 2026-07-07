--TEST--
PDF: AreaChart band-mode translucent gradient differs from an opaque band
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die("skip no system font available\n");
try {
    (new FastChart\AreaChart(80, 60))->setSeries([1, 2, 3])->renderPdf();
} catch (\Error $e) {
    if (strpos($e->getMessage(), "PDF support not compiled in") !== false) {
        die("skip extension built without --with-pdfio\n");
    }
    throw $e;
}
?>
--INI--
asan.detect_leaks=0
--FILE--
<?php
/* AreaChart band mode packs a per-band alpha into the high byte of the
 * gradient endpoints (fastchart_area.c). The PDF gradient fallback used
 * to discard it (from_rgb | 0xFF), rendering every band opaque — so a
 * translucent band and an opaque band produced identical output. Post-
 * fix the packed alpha is flattened against the captured page
 * background, so the two fills differ; the content stream differs and
 * the compressed PDF length changes. */

require __DIR__ . '/_font_candidates.inc';
$font = fc_pick_font();

function band(int $opacity, string $font): string {
    return (new FastChart\AreaChart(300, 200))
        ->setBandMode(true)->setStacked(false)
        ->setGradientFill(0xFF0000, 0x0000FF)
        ->setFillOpacity($opacity)
        ->setFontPath($font)
        ->setSeries([
            ["name" => "hi", "data" => [10, 40, 20, 50]],
            ["name" => "lo", "data" => [ 5, 20, 10, 25]],
        ])
        ->renderPdf();
}

$opaque      = band(0,  $font);   /* alpha byte 255 */
$translucent = band(64, $font);   /* alpha byte 127 -> flattened */

echo "both_valid: ",
    (str_starts_with($opaque, "%PDF-") && str_starts_with($translucent, "%PDF-")
        ? "yes" : "no"), "\n";
echo "translucent_differs_from_opaque: ",
    (strlen($opaque) !== strlen($translucent) ? "yes" : "no"), "\n";
?>
--EXPECT--
both_valid: yes
translucent_differs_from_opaque: yes
