--TEST--
round 4 hardening: non-finite float rejection, addTextAnnotation int range, per-element label NUL drop
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Non-finite floats are rejected at the float setter boundary. */
foreach ([
    ['addHorizontalLine', [INF]],
    ['addHorizontalLine', [NAN]],
    ['addVerticalLine',   [INF]],
] as [$method, $args]) {
    try {
        (new FastChart\LineChart)->$method(...$args);
        echo "$method(non-finite): no throw\n";
    } catch (\ValueError $e) {
        echo "$method(non-finite): ValueError ok\n";
    }
}

try {
    (new FastChart\PieChart)->setOtherThreshold(NAN);
    echo "setOtherThreshold(NAN): no throw\n";
} catch (\ValueError $e) {
    echo "setOtherThreshold(NAN): ValueError ok\n";
}

try {
    (new FastChart\GaugeChart)->setRange(0.0, INF);
    echo "setRange(_, INF): no throw\n";
} catch (\ValueError $e) {
    echo "setRange(_, INF): ValueError ok\n";
}

try {
    (new FastChart\LineChart)->setYAxisRange(-INF, 1.0);
    echo "setYAxisRange(-INF, _): no throw\n";
} catch (\ValueError $e) {
    echo "setYAxisRange(-INF, _): ValueError ok\n";
}

/* addTextAnnotation x/y must fit in int. */
try {
    (new FastChart\LineChart)->addTextAnnotation("ok", PHP_INT_MAX, 0);
    echo "addTextAnnotation(PHP_INT_MAX): no throw\n";
} catch (\ValueError $e) {
    echo "addTextAnnotation(PHP_INT_MAX): ValueError ok\n";
}

try {
    (new FastChart\LineChart)->addTextAnnotation("ok", 0, PHP_INT_MIN);
    echo "addTextAnnotation(PHP_INT_MIN): no throw\n";
} catch (\ValueError $e) {
    echo "addTextAnnotation(PHP_INT_MIN): ValueError ok\n";
}

/* In-range still works. */
$ok = (new FastChart\LineChart)->addTextAnnotation("hi", 100, 100);
echo "addTextAnnotation(in-range): ok\n";

/* Per-element labels with embedded NUL silently drop (do not abort draw). */
$im = imagecreatetruecolor(200, 100);
$chart = (new FastChart\BarChart)
    ->setSize(200, 100)
    ->setSeries([
        ['label' => "good", 'data' => [1, 2, 3]],
        ['label' => "bad\0label", 'data' => [4, 5, 6]],
    ]);
$chart->draw($im);
echo "BarChart NUL label: drawn ok\n";
?>
--EXPECT--
addHorizontalLine(non-finite): ValueError ok
addHorizontalLine(non-finite): ValueError ok
addVerticalLine(non-finite): ValueError ok
setOtherThreshold(NAN): ValueError ok
setRange(_, INF): ValueError ok
setYAxisRange(-INF, _): ValueError ok
addTextAnnotation(PHP_INT_MAX): ValueError ok
addTextAnnotation(PHP_INT_MIN): ValueError ok
addTextAnnotation(in-range): ok
BarChart NUL label: drawn ok
