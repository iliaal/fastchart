--TEST--
ScatterChart::setTrendLine with degree > 1 fits a polynomial regression
--EXTENSIONS--
fastchart
--FILE--
<?php

// Quadratic-shaped data.
$points = [
    [1.0,  1.1], [2.0,  2.4], [3.0,  4.5],
    [4.0,  7.2], [5.0, 11.0], [6.0, 15.5],
];

$linear = (new FastChart\ScatterChart(500, 400))
    ->setPoints($points)
    ->setTrendLine(true, 0xFF0000, 1)
    ->renderPng();

$quad = (new FastChart\ScatterChart(500, 400))
    ->setPoints($points)
    ->setTrendLine(true, 0xFF0000, 2)
    ->renderPng();

$cubic = (new FastChart\ScatterChart(500, 400))
    ->setPoints($points)
    ->setTrendLine(true, 0xFF0000, 3)
    ->renderPng();

echo "linear_renders: ", strlen($linear) > 1024 ? "ok" : "bad", "\n";
echo "quadratic_renders: ", strlen($quad) > 1024 ? "ok" : "bad", "\n";
echo "cubic_renders: ", strlen($cubic) > 1024 ? "ok" : "bad", "\n";

// Degrees should produce visually different output.
echo "quad_differs_linear: ", $quad !== $linear ? "yes" : "no", "\n";
echo "cubic_differs_quad: ",  $cubic !== $quad  ? "yes" : "no", "\n";

// Out-of-range degree.
try {
    (new FastChart\ScatterChart)->setTrendLine(true, null, 99);
    echo "bad_degree: no throw\n";
} catch (\ValueError $e) {
    echo "bad_degree: ValueError ok\n";
}
?>
--EXPECT--
linear_renders: ok
quadratic_renders: ok
cubic_renders: ok
quad_differs_linear: yes
cubic_differs_quad: yes
bad_degree: ValueError ok
