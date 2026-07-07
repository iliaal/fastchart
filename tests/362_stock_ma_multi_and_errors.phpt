--TEST--
StockChart::setMovingAverages() renders one overlay per period and filters invalid entries
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Coverage gap: setMovingAverages() had zero references. Each period
 * gets its own palette colour, so the count of distinct 2px-line
 * stroke colours equals the number of accepted overlays. Invalid
 * entries (non-int, < 2) are silently dropped by the bulk setter
 * (unlike addMovingAverage(), which throws — exercised below). */

function overlay_colors(string $svg): array {
    preg_match_all('/stroke="(#[0-9A-F]{6})" stroke-width="2"/', $svg, $m);
    return array_values(array_unique($m[1]));
}

$rows = [];
for ($i = 0; $i < 40; $i++) {
    $c = 100 + $i;
    $rows[] = [1700000000 + $i * 86400, $c, $c + 1, $c - 1, $c, 1000];
}
$mk = fn() => (new FastChart\StockChart(600, 400))
    ->setOhlcv($rows)->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);

echo "none: ",     count(overlay_colors($mk()->renderSvg())), "\n";
echo "one: ",      count(overlay_colors($mk()->setMovingAverages([5])->renderSvg())), "\n";
echo "two: ",      count(overlay_colors($mk()->setMovingAverages([5, 10])->renderSvg())), "\n";
/* Invalid entries (non-int "x", 0, -3) dropped; 5 and 10 survive. */
echo "filtered: ", count(overlay_colors($mk()->setMovingAverages([5, "x", 10, 0, -3])->renderSvg())), "\n";

/* addMovingAverage() rejects a below-range period and an unknown type. */
try {
    $mk()->addMovingAverage(1);
    echo "period: no throw\n";
} catch (\ValueError $e) {
    echo "period: ", $e->getMessage(), "\n";
}
try {
    $mk()->addMovingAverage(5, 99);
    echo "type: no throw\n";
} catch (\ValueError $e) {
    echo "type: ", $e->getMessage(), "\n";
}
?>
--EXPECT--
none: 0
one: 1
two: 2
filtered: 2
period: FastChart\StockChart::addMovingAverage() period must be >= 2
type: FastChart\StockChart::addMovingAverage() type must be MA_SMA, MA_EMA or MA_WMA
