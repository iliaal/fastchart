--TEST--
setXLabelStride renders only every Nth X-axis label
--EXTENSIONS--
fastchart
--FILE--
<?php

// Larger stride should reduce the number of labels rendered. We
// don't pixel-count labels (they overlap with bars) -- just make
// sure both stride values produce a valid render and the setter
// rejects bad input.
foreach ([1, 2, 5] as $s) {
    $b = (new FastChart\BarChart(700, 400))
        ->setXLabelStride($s)
        ->setSeries([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
        ->setCategoryLabels(['A','B','C','D','E','F','G','H','I','J'])
        ->renderPng();
    echo "stride_$s: ", strlen($b) > 1024 ? "ok" : "bad", "\n";
}

try {
    (new FastChart\BarChart)->setXLabelStride(0);
    echo "zero: no throw\n";
} catch (\ValueError $e) {
    echo "zero: ValueError ok\n";
}
try {
    (new FastChart\BarChart)->setXLabelStride(2000);
    echo "huge: no throw\n";
} catch (\ValueError $e) {
    echo "huge: ValueError ok\n";
}
?>
--EXPECT--
stride_1: ok
stride_2: ok
stride_5: ok
zero: ValueError ok
huge: ValueError ok
