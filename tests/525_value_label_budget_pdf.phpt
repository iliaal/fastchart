--TEST--
PDF output applies the per-render value-label budget
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
try {
    (new FastChart\LineChart(80, 60))->setSeries([1, 2, 3])->renderPdf();
} catch (\Error $e) {
    if (strpos($e->getMessage(), "PDF support not compiled in") !== false) {
        die("skip PDF support not compiled in\n");
    }
}
?>
--FILE--
<?php
/* '.' keeps each glyph outline tiny so the capped render stays cheap. */
$fmt = str_repeat('.', 8000) . '%.0f';
$chart = fn(int $n) => (new FastChart\LineChart(800, 400))
    ->setSeries(array_fill(0, $n, 5))->setShowValues(true, $fmt);

/* 8 labels of 8001 bytes fit; 16 points still draw only 8 of them, so
 * the PDF grows only by the extra markers and line segments. */
$fits = strlen($chart(8)->renderPdf());
$capped = $chart(16)->renderPdf();
echo "pdf: ", str_starts_with($capped, '%PDF-') ? "ok" : "bad", "\n";
echo "capped size: ", strlen($capped) < $fits * 1.25
    ? "bounded" : "grew from $fits to " . strlen($capped), "\n";

try {
    $chart(16)->setStrict(true)->renderPdf();
    echo "strict: no exception\n";
} catch (ValueError $e) {
    echo "strict: ", $e->getMessage(), "\n";
}
echo "strict at budget: ", str_starts_with(
    $chart(8)->setStrict(true)->renderPdf(), '%PDF-') ? "ok" : "bad", "\n";
?>
--EXPECT--
pdf: ok
capped size: bounded
strict: FastChart: per-element labels exceed the 65536-byte per-render text limit
strict at budget: ok
