--TEST--
Format-string validators reject %s / %n / %d / multi-conversion / length-modified specs
--EXTENSIONS--
fastchart
--FILE--
<?php

// Each format-taking setter must reject anything that isn't a single
// double-compatible conversion. Untrusted format strings could
// otherwise read uninitialized integer registers or write-through
// pointers via %n.
$bad_formats = [
    '%s',         // wrong arg type -- crash
    '%n',         // write-where
    '%d',         // wrong arg type (int from a double slot)
    '%i',
    '%x',
    '%5.2f%5.2f', // two conversions
    'plain text', // zero conversions
    '%lf',        // length modifier rejected
    '%hd',
    '%',          // incomplete
];

$setters = [
    [new FastChart\LineChart, 'setShowValues',     [true, '__FMT__']],
    [new FastChart\LineChart, 'setYAxisLabelFormat',[      '__FMT__']],
    [new FastChart\LineChart, 'setXAxisLabelFormat',[      '__FMT__']],
    [new FastChart\PieChart,  'setSliceLabelFormat',[      '__FMT__']],
    [new FastChart\GaugeChart,'setValueFormat',     [      '__FMT__']],
    [new FastChart\SurfaceChart,'setShowCellValues',[true, '__FMT__']],
];

$pass = 0; $fail = 0;
foreach ($setters as [$obj, $method, $args]) {
    foreach ($bad_formats as $bad) {
        $a = $args;
        foreach ($a as &$v) if ($v === '__FMT__') $v = $bad;
        unset($v);
        try {
            $obj->$method(...$a);
            echo "MISS: $method($bad) accepted\n";
            $fail++;
        } catch (\ValueError $e) {
            $pass++;
        }
    }
}
echo "rejected: $pass / ", count($setters) * count($bad_formats), "\n";
echo "missed: $fail\n";

// Good formats must still pass.
$good = ['%g', '%.2f', '%.0f', '%5.2f', 'val: %.2f%%', '%e', '%G'];
foreach ($good as $g) {
    try {
        (new FastChart\LineChart)->setShowValues(true, $g);
        echo "good '$g': ok\n";
    } catch (\ValueError $e) {
        echo "good '$g': REJECTED ($e->getMessage())\n";
    }
}

// Embedded NUL still rejected.
try {
    (new FastChart\LineChart)->setYAxisLabelFormat("\$%.2f\0evil");
    echo "nul: not rejected\n";
} catch (\ValueError $e) {
    echo "nul: rejected ok\n";
}

// Empty string is the "clear" sentinel and must NOT trigger validation.
$out = (new FastChart\LineChart)
    ->setShowValues(true, '%g')
    ->setShowValues(true, '');
var_dump($out instanceof FastChart\LineChart);
?>
--EXPECT--
rejected: 60 / 60
missed: 0
good '%g': ok
good '%.2f': ok
good '%.0f': ok
good '%5.2f': ok
good 'val: %.2f%%': ok
good '%e': ok
good '%G': ok
nul: rejected ok
bool(true)
