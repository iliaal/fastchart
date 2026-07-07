--TEST--
StockChart::addVWAP() overlay is flat for constant input and matches cumulative VWAP for varying volume
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Coverage gap: VWAP was only well-formed-SVG-checked. The C uses a
 * cumulative typical-price VWAP with typical = (high+low+close)/3 and
 * running sum(typical*volume)/sum(volume). Constant typical + constant
 * volume gives a flat line at that price; a varying-volume fixture is
 * matched pixel-for-pixel against a PHP reference through a forced
 * (exact) y-axis. All prices strictly positive. */

function ov_points(string $svg): array {
    preg_match_all(
        '/<line x1="([-0-9.]+)" y1="([-0-9.]+)" x2="([-0-9.]+)" y2="([-0-9.]+)" stroke="[^"]*" stroke-width="2"/',
        $svg, $m, PREG_SET_ORDER);
    $pts = [];
    if ($m) {
        $pts[] = [(float)$m[0][1], (float)$m[0][2]];
        foreach ($m as $s) $pts[] = [(float)$s[3], (float)$s[4]];
    }
    return $pts;
}
function plot_rect(string $svg): array {
    preg_match_all('/<rect x="[0-9.-]+" y="([0-9.-]+)" width="[0-9.-]+" height="([0-9.-]+)" fill="#FFFFFF"/',
        $svg, $r, PREG_SET_ORDER);
    $p = $r[1]; $y0 = (float)$p[1]; $h = (float)$p[2] - 1;
    return [$y0, $y0 + $h, $h];
}
function ypix(float $v, float $min, float $max, array $pr): int {
    [$y0, $y1, $h] = $pr;
    $f = ($v - $min) / ($max - $min);
    if ($f < 0) $f = 0; if ($f > 1) $f = 1;
    return (int)($y1 - (int)($f * $h + 0.5));
}

/* --- Flat: constant typical price (100) and constant volume. --- */
$flat = [];
for ($i = 0; $i < 20; $i++) {
    $flat[] = [1700000000 + $i * 86400, 100.0, 102.0, 98.0, 100.0, 1000];
}
$svg = (new FastChart\StockChart(600, 400))
    ->setOhlcv($flat)->setYAxisRange(90.0, 110.0)->addVWAP()
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)->renderSvg();
$pts = ov_points($svg); $pr = plot_rect($svg);
$ys = array_unique(array_column($pts, 1));
echo "flat: ", (count($pts) > 10 && count($ys) === 1 ? 'flat' : 'NOT FLAT'),
     " ", (count($ys) === 1 && (int)reset($ys) === ypix(100.0, 90.0, 110.0, $pr) ? 'at_price' : 'OFF'), "\n";

/* --- Varying volume: exact cumulative VWAP match. --- */
$MIN = 90.0; $MAX = 130.0;
$rows = [];
for ($i = 0; $i < 15; $i++) {
    $close = 100 + ($i % 5) * 3;                 /* strictly positive */
    $rows[] = [1700000000 + $i * 86400, $close, $close + 2, $close - 2, $close, 1000 + ($i * 137 % 900)];
}
$svg = (new FastChart\StockChart(600, 400))
    ->setOhlcv($rows)->setYAxisRange($MIN, $MAX)->addVWAP()
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)->renderSvg();
$pts = ov_points($svg); $pr = plot_rect($svg);

$cum_pv = 0.0; $cum_v = 0.0; $ref = [];
foreach ($rows as $r) {
    $tp = ($r[2] + $r[3] + $r[4]) / 3.0;
    $cum_pv += $tp * $r[5]; $cum_v += $r[5];
    $ref[] = $cum_pv / $cum_v;
}
$maxdiff = 0;
foreach ($pts as $k => $p) {
    $maxdiff = max($maxdiff, abs((int)$p[1] - ypix($ref[$k], $MIN, $MAX, $pr)));
}
echo "varying: npts=", count($pts), " maxdiff=$maxdiff\n";
?>
--EXPECT--
flat: flat at_price
varying: npts=15 maxdiff=0
