--TEST--
BoxPlot: quartile boxes with whiskers, median line, optional outliers
--EXTENSIONS--
fastchart
--FILE--
<?php

// Mixed input shapes: flat 5-tuple and dict with outliers.
$bytes = (new FastChart\BoxPlot(600, 400))
    ->setBoxes([
        [10, 25, 35, 45, 60],                          // [min, q1, med, q3, max]
        ['min' => 5,  'q1' => 20, 'median' => 30,
         'q3' => 40, 'max' => 55, 'outliers' => [2, 70], 'label' => 'B'],
        [15, 28, 38, 48, 65],
    ])
    ->setCategoryLabels(['A', 'B', 'C'])
    ->renderPng();
echo "renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

// Box width.
$bytes2 = (new FastChart\BoxPlot(500, 400))
    ->setBoxes([[10, 20, 30, 40, 50]])
    ->setBoxWidth(30)
    ->renderPng();
echo "narrow: ", strlen($bytes2) > 1024 ? "ok" : "bad", "\n";

// Out-of-range width rejected.
try {
    (new FastChart\BoxPlot)->setBoxWidth(200);
    echo "bad: no throw\n";
} catch (\ValueError $e) {
    echo "bad: ValueError ok\n";
}

// Empty data.
try {
    (new FastChart\BoxPlot(400, 300))->setBoxes([])->renderPng();
    echo "empty: no throw\n";
} catch (\Error $e) {
    echo "empty: error ok\n";
}
?>
--EXPECT--
renders: ok
narrow: ok
bad: ValueError ok
empty: error ok
