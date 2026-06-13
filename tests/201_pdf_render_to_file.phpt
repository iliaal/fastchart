--TEST--
Chart::renderToFile('*.pdf'): writes a vector PDF, infers format from extension
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
$path = __DIR__ . "/201_pdf_out.pdf";
@unlink($path);

$c = (new FastChart\BarChart(480, 320))
    ->setSeries([["label" => "x", "data" => [3, 7, 4, 8, 6]]])
    ->setTitle("bars");

$written = $c->renderToFile($path);

var_dump(is_int($written));
var_dump($written > 500);
var_dump(filesize($path) === $written);

$bytes = file_get_contents($path);
var_dump(str_starts_with($bytes, "%PDF-"));
var_dump(str_contains($bytes, "startxref"));
var_dump(str_contains($bytes, "/MediaBox[ 0 0 480 320]"));

@unlink($path);
echo "done\n";
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
done
