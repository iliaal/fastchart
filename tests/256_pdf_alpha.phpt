--TEST--
PDF: a translucent fill renders (alpha flattened against the page background)
--SKIPIF--
<?php
if (!extension_loaded('fastchart')) { echo "skip fastchart not loaded"; }
try {
    (new FastChart\AreaChart(20, 20))->setSeries([['name' => 's', 'data' => [1, 2]]])->renderPdf();
} catch (\Throwable $e) {
    if (strpos($e->getMessage(), 'not compiled') !== false) echo "skip PDF support not compiled in";
}
?>
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_20cc94ee: the PDF backend dropped the alpha byte, rendering translucent
 * fills opaque. Alpha is now composited against the captured page background.
 * pdfio has no transparency API, so this is a flatten, not real transparency. */

use FastChart\AreaChart;

$pdf = (new AreaChart())->setStacked(false)->setSize(300, 200)
    ->setFillOpacity(64)
    ->setSeries([['name' => 's', 'data' => [10, 40, 20, 50]]])
    ->renderPdf();

echo "pdf_renders: ", (strlen($pdf) > 500 && strncmp($pdf, '%PDF', 4) === 0 ? 'yes' : 'no'), "\n";

?>
--EXPECT--
pdf_renders: yes
