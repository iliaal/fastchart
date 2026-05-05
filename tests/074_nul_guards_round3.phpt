--TEST--
addHorizontalLine / addVerticalLine / addTextAnnotation / addIndicatorPane reject embedded NUL
--EXTENSIONS--
fastchart
--FILE--
<?php

foreach ([
    [['addHorizontalLine', [1.0, "bad\0label"]],            'addHorizontalLine'],
    [['addVerticalLine',   [1.0, "bad\0label"]],            'addVerticalLine'],
    [['addTextAnnotation', ["bad\0text", 10, 10]],          'addTextAnnotation'],
] as [$call, $name]) {
    [$method, $args] = $call;
    try {
        (new FastChart\LineChart)->$method(...$args);
        echo "$name: no throw\n";
    } catch (\ValueError $e) {
        echo "$name: ValueError ok\n";
    }
}

try {
    (new FastChart\StockChart)->addIndicatorPane("bad\0name", [1, 2, 3]);
    echo "addIndicatorPane: no throw\n";
} catch (\ValueError $e) {
    echo "addIndicatorPane: ValueError ok\n";
}
?>
--EXPECT--
addHorizontalLine: ValueError ok
addVerticalLine: ValueError ok
addTextAnnotation: ValueError ok
addIndicatorPane: ValueError ok
