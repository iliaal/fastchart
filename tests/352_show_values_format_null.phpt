--TEST--
Chart::setShowValues(null) preserves the format; '' resets; omitted preserves
--EXTENSIONS--
fastchart
--FILE--
<?php

/* The stub advertised `string $format = '%g'`, but the impl preserves the
 * prior format when the argument is absent and treats '' as reset. The
 * signature is now `?string $format = null`: null (or omitted) leaves the
 * current format untouched, '' resets to the default, a non-empty string
 * sets it. Before the fix, passing null threw a TypeError. */

use FastChart\BarChart;
use FastChart\Chart;

function value_label(array $calls): string {
    $c = (new BarChart(400, 300))
        ->setSvgTextMode(Chart::SVG_TEXT_NATIVE)
        ->setSeries([["name" => "s", "data" => [1.5]]]);
    foreach ($calls as $call) {
        $call($c);
    }
    $svg = $c->renderSvg();
    // The datapoint label is the value 1.5 rendered through the active
    // format; "%.3f" -> "1.500", "%g" -> "1.5".
    if (strpos($svg, '>1.500<') !== false) return "1.500";
    if (strpos($svg, '>1.5<') !== false) return "1.5";
    return "none";
}

$set = fn($c) => $c->setShowValues(true, "%.3f");

echo "then null:  ", value_label([$set, fn($c) => $c->setShowValues(true, null)]), "\n";
echo "then omit:  ", value_label([$set, fn($c) => $c->setShowValues(true)]), "\n";
echo "then empty: ", value_label([$set, fn($c) => $c->setShowValues(true, "")]), "\n";
echo "only fmt:   ", value_label([$set]), "\n";

?>
--EXPECT--
then null:  1.500
then omit:  1.500
then empty: 1.5
only fmt:   1.500
