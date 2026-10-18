--TEST--
MarimekkoChart::setColumns() rejects over-budget labels before copying them
--EXTENSIONS--
fastchart
--INI--
memory_limit=32M
--FILE--
<?php
/* One shared 8 KiB string: the input costs almost nothing, while copying
 * every label (128 columns x 65 labels) would need about 68 MB. */
$k = str_repeat('M', 8192);
$cols = array_fill(0, 128, [
    'label' => $k,
    'segments' => array_fill(0, 64, ['label' => $k, 'value' => 1]),
]);
$c = (new FastChart\MarimekkoChart(300, 200))
    ->setColumns([['label' => 'keep', 'segments' => [['value' => 1]]]]);
$before = $c->renderSvg();
if (function_exists('memory_reset_peak_usage')) memory_reset_peak_usage();
$base = memory_get_usage();
try {
    $c->setColumns($cols);
    echo "accepted\n";
} catch (ValueError $e) {
    echo $e->getMessage(), "\n";
}
echo "state: ", $c->renderSvg() === $before ? "preserved" : "CHANGED", "\n";
if (getenv('USE_ZEND_ALLOC') !== '0') {
    $peak = memory_get_peak_usage() - $base;
    echo "peak: ", $peak < 4 * 1024 * 1024 ? "bounded" : "$peak bytes", "\n";
} else {
    echo "peak: bounded\n";
}
?>
--EXPECT--
FastChart\MarimekkoChart::setColumns() aggregate text exceeds the 65536-byte limit
state: preserved
peak: bounded
