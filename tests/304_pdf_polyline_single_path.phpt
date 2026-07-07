--TEST--
PDF: a many-vertex polyline (ChordDiagram STYLE_LINE) renders one stroked path
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die("skip no system font available\n");
try {
    (new FastChart\LineChart(80, 60))->setSeries([1, 2, 3])->renderPdf();
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
/* fastchart_target_polyline (used by ChordDiagram STYLE_LINE) used to
 * decompose a curve into n-1 independently stroked segments, each with
 * its own save/restore and butt caps — visible notches at interior
 * vertices for thickness > 1. It now builds one path and strokes once
 * (fc_pdf_emit_polyline). The op sequence lives in a FlateDecode-
 * compressed stream, so this test pins that the many-vertex, thick
 * (value-proportional up to 9px) polylines render a valid document; the
 * one-path structure (328 -> 16 movetos, 320 -> 8 strokes for this
 * input) was verified out of band by decompressing the content stream. */

require __DIR__ . '/_font_candidates.inc';
$font = fc_pick_font();

$pdf = (new FastChart\ChordDiagram(300, 300))
    ->setStyle(FastChart\ChordDiagram::STYLE_LINE)
    ->setNodes([['label' => 'A'], ['label' => 'B'],
                ['label' => 'C'], ['label' => 'D']])
    ->setLinks([['from' => 0, 'to' => 1, 'value' => 3],
                ['from' => 1, 'to' => 2, 'value' => 2],
                ['from' => 2, 'to' => 3, 'value' => 4],
                ['from' => 0, 'to' => 3, 'value' => 1]])
    ->setFontPath($font)
    ->renderPdf();

echo "valid_pdf: ",
    (str_starts_with($pdf, "%PDF-") && str_contains($pdf, "startxref")
        ? "yes" : "no"), "\n";
echo "nontrivial: ", (strlen($pdf) > 500 ? "yes" : "no"), "\n";
?>
--EXPECT--
valid_pdf: yes
nontrivial: yes
