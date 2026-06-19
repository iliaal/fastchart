--TEST--
Array setters accept foreach-by-reference (IS_REFERENCE) buckets
--EXTENSIONS--
fastchart
simplexml
--INI--
asan.detect_leaks=0
--FILE--
<?php

/* `foreach ($a as &$v) { ... }` leaves every visited bucket as an
 * IS_REFERENCE wrapper, and the references survive the loop. A parser
 * that inspects Z_TYPE_P(bucket) without ZVAL_DEREF rejects the
 * wrapper and silently drops valid data. Pass each family the same
 * data twice -- once untouched, once after recursively converting every
 * bucket to a reference -- and require byte-identical, well-formed SVG. */

function deref_all(&$a) {
    if (is_array($a)) {
        foreach ($a as &$x) { deref_all($x); }
        unset($x);
    }
}

function check($label, $data, $render) {
    $baseline = $render($data);
    $reffed = $data;
    deref_all($reffed);
    $byref = $render($reffed);

    libxml_use_internal_errors(true);
    $valid = simplexml_load_string($byref) instanceof SimpleXMLElement;

    $ok = $valid && $byref === $baseline && strlen($byref) > 200;
    echo $label, ": ", ($ok ? "ok" : "FAIL"), "\n";
}

check("line",
    [['label' => 's1', 'data' => [3, 1, 4, 1, 5, 9, 2, 6], 'color' => 0x336699]],
    fn($d) => (new FastChart\LineChart(480, 320))->setSeries($d)->renderSvg());

check("scatter",
    [[1, 2], [3, 5], [4, 4], [6, 8], [7, 6]],
    fn($d) => (new FastChart\ScatterChart(480, 320))->setPoints($d)->renderSvg());

check("pie",
    [['label' => 'A', 'value' => 30, 'color' => 0xCC4444],
     ['label' => 'B', 'value' => 50],
     ['label' => 'C', 'value' => 20]],
    fn($d) => (new FastChart\PieChart(360, 360))->setSlices($d)->renderSvg());

$ohlcv = [];
$t = strtotime('2025-03-01');
for ($i = 0; $i < 20; $i++) {
    $ohlcv[] = [$t + $i * 86400, 100 + $i, 108 + $i, 95 + $i, 103 + $i, 1000 + $i * 10];
}
check("stock", $ohlcv,
    fn($d) => (new FastChart\StockChart(560, 320))->setOhlcv($d)->renderSvg());

check("sunburst",
    ['label' => 'root', 'children' => [
        ['label' => 'a', 'color' => 0x6C8EBF, 'children' => [
            ['label' => 'a1', 'value' => 4], ['label' => 'a2', 'value' => 6]]],
        ['label' => 'b', 'value' => 8, 'color' => 0x82B366],
    ]],
    fn($d) => (new FastChart\SunburstChart(400, 400))->setHierarchy($d)->renderSvg());

check("heatmap",
    [[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12]],
    fn($d) => (new FastChart\Heatmap(420, 300))->setGrid($d)->renderSvg());

echo "OK\n";
--EXPECT--
line: ok
scatter: ok
pie: ok
stock: ok
sunburst: ok
heatmap: ok
OK
