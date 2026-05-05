--TEST--
setXAxisFont / setYAxisFont / setXAxisTitleFont / setYAxisTitleFont / setAnnotationFont
--EXTENSIONS--
fastchart
--FILE--
<?php

// Sizes propagate to render without crashing.
$bytes = (new FastChart\LineChart(600, 400))
    ->setXAxisTitle('X axis')
    ->setYAxisTitle('Y axis')
    ->setXAxisFont(null, 8.0)
    ->setYAxisFont(null, 9.0)
    ->setXAxisTitleFont(null, 14.0)
    ->setYAxisTitleFont(null, 14.0)
    ->setAnnotationFont(null, 7.0)
    ->setSeries([1, 5, 3, 8])
    ->addTextAnnotation('hello', 200, 200)
    ->renderPng();
echo "renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

// Out-of-range size rejected on each.
foreach (['setXAxisFont', 'setYAxisFont', 'setXAxisTitleFont', 'setYAxisTitleFont', 'setAnnotationFont'] as $m) {
    try {
        (new FastChart\LineChart)->$m(null, 0.5);
        echo "$m bad_size: no throw\n";
    } catch (\ValueError $e) {
        echo "$m bad_size: ValueError ok\n";
    }
}

// Embedded NUL in path rejected on one (the macro path is shared).
try {
    (new FastChart\LineChart)->setXAxisFont("/etc\0/passwd");
    echo "nul: no throw\n";
} catch (\ValueError $e) {
    echo "nul: ValueError ok\n";
}
?>
--EXPECT--
renders: ok
setXAxisFont bad_size: ValueError ok
setYAxisFont bad_size: ValueError ok
setXAxisTitleFont bad_size: ValueError ok
setYAxisTitleFont bad_size: ValueError ok
setAnnotationFont bad_size: ValueError ok
nul: ValueError ok
