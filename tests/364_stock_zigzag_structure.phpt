--TEST--
StockChart::addZigZag() pivots on threshold-exceeding reversals and ignores sub-threshold wiggles
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Coverage gap: ZigZag was only well-formed-SVG-checked. The pivot
 * line connects confirmed pivots (2px, the only such line here).
 * Series A rises, reverses down past the 10% threshold, then rises
 * past it again — four pivots: start(100), peak(110), trough(95),
 * final(117); three segments. Series B has the same rise with a
 * sub-threshold dip (110 -> 109, ~0.9%) that must NOT create a pivot,
 * leaving only start and final (one segment). All prices positive. */

function zz(string $svg): array {
    preg_match_all(
        '/<line x1="([-0-9.]+)" y1="([-0-9.]+)" x2="([-0-9.]+)" y2="([-0-9.]+)" stroke="[^"]*" stroke-width="2"/',
        $svg, $m, PREG_SET_ORDER);
    $segments = count($m);
    $pts = [];
    if ($m) {
        $pts["{$m[0][1]},{$m[0][2]}"] = true;
        foreach ($m as $s) $pts["{$s[3]},{$s[4]}"] = true;
    }
    return [$segments, count($pts)];   /* [segments, distinct vertices] */
}

$mk = function (array $closes): FastChart\StockChart {
    $rows = [];
    foreach ($closes as $i => $c) {
        $rows[] = [1700000000 + $i * 86400, $c, $c + 0.5, $c - 0.5, $c, 1000];
    }
    return (new FastChart\StockChart(600, 400))->setOhlcv($rows)
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
};

/* A: rise 100..110, drop to 95 (>10% of 110), rise to 117 (>10% of 95). */
$A = [];
for ($i = 0; $i <= 10; $i++) $A[] = 100 + $i;
$A[] = 95; $A[] = 115; $A[] = 116; $A[] = 117;

/* B: rise 100..110, dip to 109 (sub-threshold), keep rising to 120. */
$B = [];
for ($i = 0; $i <= 10; $i++) $B[] = 100 + $i;
$B[] = 109;
for ($i = 0; $i < 10; $i++) $B[] = 111 + $i;

[$sa, $va] = zz($mk($A)->addZigZag(10.0)->renderSvg());
[$sb, $vb] = zz($mk($B)->addZigZag(10.0)->renderSvg());

echo "A_vertices: $va\n";
echo "A_segments: $sa\n";
echo "B_vertices: $vb\n";
echo "B_segments: $sb\n";
echo "reversal_adds_pivots: ", ($va > $vb ? 'yes' : 'NO'), "\n";
?>
--EXPECT--
A_vertices: 4
A_segments: 3
B_vertices: 2
B_segments: 1
reversal_adds_pivots: yes
