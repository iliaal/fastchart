--TEST--
FastChart objects forbid dynamic properties and serialization
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Chart and Symbol state lives in a native C struct, not PHP
 * properties. @strict-properties makes a stray property assignment a
 * hard Error instead of a silent deprecation + JSON leak, and
 * @not-serializable makes serialize() throw instead of emitting a
 * state-less husk that unserializes into a blank object. Both flags
 * apply across the whole class hierarchy (not-serializable inherits
 * from the abstract base; strict-properties is set per final class). */

function dynprop_blocked(object $o): bool {
    try { $o->nope = 1; return false; }
    catch (\Error $e) { return true; }
}
function serialize_blocked(object $o): bool {
    try { serialize($o); return false; }
    catch (\Exception $e) { return true; }
}

$cases = [
    'LineChart'  => new FastChart\LineChart(100, 100),
    'PieChart'   => new FastChart\PieChart(100, 100),
    'StockChart' => new FastChart\StockChart(100, 100),
    'Code128'    => new FastChart\Code128(),
    'QrCode'     => new FastChart\QrCode(),
];

foreach ($cases as $name => $obj) {
    echo $name, ": dynprop=", dynprop_blocked($obj) ? "blocked" : "OPEN",
         " serialize=", serialize_blocked($obj) ? "blocked" : "OPEN", "\n";
}

echo "ok\n";
?>
--EXPECT--
LineChart: dynprop=blocked serialize=blocked
PieChart: dynprop=blocked serialize=blocked
StockChart: dynprop=blocked serialize=blocked
Code128: dynprop=blocked serialize=blocked
QrCode: dynprop=blocked serialize=blocked
ok
