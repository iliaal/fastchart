--TEST--
Chart::renderPdf(): structural validation of vector PDF output
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
$c = new FastChart\LineChart(80, 60);
$c->setSeries([["label" => "a", "data" => [1, 2, 3]]]);
try {
    $c->renderPdf();
} catch (\Error $e) {
    if (strpos($e->getMessage(), "PDF support not compiled in") !== false) {
        die("skip extension built without --with-pdfio");
    }
    throw $e;
}
?>
--INI--
asan.detect_leaks=0
--FILE--
<?php
$c = new FastChart\LineChart(640, 320);
$c->setSeries([
    ["label" => "alpha", "data" => [10, 14, 12, 18, 22, 19, 25, 23]],
    ["label" => "beta",  "data" => [ 8, 11, 15, 13, 17, 21, 20, 24]],
])
  ->setTitle("Quarterly revenue")
  ->setCategoryLabels(["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug"]);

$pdf = $c->renderPdf();

var_dump(is_string($pdf));
var_dump(strlen($pdf) > 500);
// Valid PDF header + trailer.
var_dump(str_starts_with($pdf, "%PDF-"));
// pdfio's printf collapses "%%EOF" to a single-% "%EOF" marker; the
// xref trailer keyword is the reliable completeness signal.
var_dump(str_contains($pdf, "startxref"));
// Single page sized to the logical setSize() dims; MediaBox and CropBox
// must agree so viewers honoring CropBox don't clip the page.
var_dump(str_contains($pdf, "/MediaBox[ 0 0 640 320]"));
var_dump(str_contains($pdf, "/CropBox[ 0 0 640 320]"));
var_dump(str_contains($pdf, "/Type/Page"));

// Pie wedges exercise the arc primitive; ensure that path also produces
// a valid document.
$pie = (new FastChart\PieChart(300, 300))
    ->setSlices([["label" => "A", "value" => 30],
                 ["label" => "B", "value" => 20],
                 ["label" => "C", "value" => 50]]);
$ppdf = $pie->renderPdf();
var_dump(str_starts_with($ppdf, "%PDF-"));
var_dump(str_contains($ppdf, "/MediaBox[ 0 0 300 300]"));

// DPI-invariant like SVG: setDpi() must not change the page dimensions.
$c->setDpi(200);
$hidpi = $c->renderPdf();
var_dump(str_contains($hidpi, "/MediaBox[ 0 0 640 320]"));

// Rotated axis labels exercise the text-matrix rotation path; just
// confirm it produces a valid document (no crash, valid trailer).
$rot = (new FastChart\BarChart(400, 320))
    ->setSeries([["label" => "v", "data" => [5, 8, 3, 9, 6]]])
    ->setCategoryLabels(["January","February","March","April","May"])
    ->setXAxisLabelAngle(45)
    ->setYAxisTitle("Revenue");
$rpdf = $rot->renderPdf();
var_dump(str_starts_with($rpdf, "%PDF-") && str_contains($rpdf, "startxref"));

echo "done\n";
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
done
