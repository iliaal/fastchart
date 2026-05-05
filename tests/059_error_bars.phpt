--TEST--
LineChart / ScatterChart::setErrorBars draws ± stems with caps
--EXTENSIONS--
fastchart
--FILE--
<?php

// Reference vs. with-error-bars: the byte stream must differ.
$ref = (new FastChart\LineChart(500, 400))
    ->setSeries([10, 25, 18, 30, 22])
    ->renderPng();

$with = (new FastChart\LineChart(500, 400))
    ->setSeries([10, 25, 18, 30, 22])
    ->setErrorBars([2, 3, 1.5, [4, 6], 2])
    ->renderPng();

echo "line_renders: ",  strlen($with) > 1024 ? "ok" : "bad", "\n";
echo "line_changes: ",  ($with !== $ref ? "yes" : "no"), "\n";

// Scatter with symmetric and asymmetric error bars.
$bytes = (new FastChart\ScatterChart(500, 400))
    ->setPoints([[1, 10], [2, 12], [3, 15], [4, 18]])
    ->setErrorBars([0.5, 1.0, [0.3, 0.8], 0.6])
    ->renderPng();
echo "scatter_renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

// Empty array clears.
$out = (new FastChart\ScatterChart)->setErrorBars([]);
var_dump($out instanceof FastChart\ScatterChart);
?>
--EXPECT--
line_renders: ok
line_changes: yes
scatter_renders: ok
bool(true)
