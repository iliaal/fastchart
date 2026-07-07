--TEST--
addOverlaySeries: parsed at setter time (no GC cycle; non-numeric = gap)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* The overlay values used to be stashed as a raw PHP array inside the
 * chart's C-struct config zval, which the engine's cycle collector
 * cannot see. An object reachable through the values array that also
 * referenced the chart formed an uncollectable cycle. Parsing the
 * values into a typed C array at setter time drops the user zval, so
 * the cycle no longer exists. */
$chart = new FastChart\StockChart(400, 300);
$obj = new stdClass();
$obj->chart = $chart;                       // object -> chart
$chart->addOverlaySeries('line', [1.0, $obj, 3.0]); // no retained zval
$weak = WeakReference::create($chart);
unset($chart, $obj);
gc_collect_cycles();
echo "chart collected: ", ($weak->get() === null ? "yes" : "no"), "\n";

/* A non-numeric entry is a gap, exactly like a null. The two renders
 * must be byte-identical. */
function overlay($values) {
    return (new FastChart\BarChart(400, 300))
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setSeries([10, 20, 15])
        ->addOverlaySeries('line', $values)
        ->renderSvg();
}
$with_null   = overlay([12, null, 14]);
$with_string = overlay([12, 'oops', 14]);
echo "non-numeric is a gap: ",
     (md5($with_null) === md5($with_string) ? "yes" : "no"), "\n";

/* A finite numeric overlay differs from the all-gap one (the line is
 * actually drawn). */
$numeric = overlay([12, 13, 14]);
echo "numeric overlay draws: ",
     (md5($numeric) !== md5($with_null) ? "yes" : "no"), "\n";

?>
--EXPECT--
chart collected: yes
non-numeric is a gap: yes
numeric overlay draws: yes
