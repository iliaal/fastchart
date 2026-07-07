--TEST--
PDF: a fully-transparent drop shadow (setShadowAlpha(127)) paints nothing
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die("skip no system font available\n");
try {
    (new FastChart\BarChart(80, 60))->setSeries([1, 2, 3])->renderPdf();
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
/* setShadowAlpha(127) is libgd's "fully transparent" — fastchart_effects.c
 * maps it to an alpha byte of 0, and the SVG backend emits rgba(...,0),
 * which renders invisibly. The PDF backend previously treated alpha 0 as
 * opaque and drew the shadow solid. Post-fix a fully-transparent shape
 * emits no path at all, so the PDF is byte-for-byte the same size as the
 * identical chart with no shadow, and strictly smaller than the same
 * chart with a visible (opaque) shadow. */

require __DIR__ . '/_font_candidates.inc';
$font = fc_pick_font();

function bar(int $font_arg = 0): FastChart\BarChart {
    return (new FastChart\BarChart(200, 150))
        ->setSeries([["label" => "v", "data" => [3, 7, 4]]]);
}

$no_shadow = bar()->setFontPath($font)->renderPdf();

$invisible = bar()->setFontPath($font)
    ->setDropShadow(5, 5, 0x000000)->setShadowAlpha(127)->renderPdf();

$opaque = bar()->setFontPath($font)
    ->setDropShadow(5, 5, 0x000000)->setShadowAlpha(0)->renderPdf();

/* Fixed-width CreationDate keeps the byte length stable for identical
 * drawings, so length is a reliable skip signal. */
echo "invisible_shadow_paints_nothing: ",
    (strlen($invisible) === strlen($no_shadow) ? "yes" : "no"), "\n";
echo "opaque_shadow_paints_something: ",
    (strlen($opaque) > strlen($no_shadow) ? "yes" : "no"), "\n";
?>
--EXPECT--
invisible_shadow_paints_nothing: yes
opaque_shadow_paints_something: yes
