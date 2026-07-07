--TEST--
StockChart moving averages: SMA/EMA/WMA overlays match hand-computed values on a step series
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Coverage gap: no test pinned MA overlay numerics, so a broken EMA
 * or WMA producing plausible finite pixels would pass. Fixture: 20
 * bars at close 100 then 20 at 200 (a clean step). Reference SMA,
 * EMA (seeded with the leading SMA, alpha = 2/(P+1)) and WMA (linear
 * weights 1..P) are computed here in plain PHP mirroring the C
 * definitions, then mapped through a forced, known y-axis so the
 * overlay pixel must equal the reference pixel bar-for-bar. The axis
 * is forced with setYAxisRange so the mapping is exact (no reliance
 * on the auto "nice range" rounding). A shortly-post-step responsive-
 * ness check confirms EMA reacts faster than SMA. */

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

$P = 10; $MIN = 50.0; $MAX = 250.0;
$rows = [];
for ($i = 0; $i < 40; $i++) {
    $c = $i < 20 ? 100.0 : 200.0;
    $rows[] = [1700000000 + $i * 86400, $c, $c + 1, $c - 1, $c, 1000];
}
$closes = array_column($rows, 4);

/* Reference MAs, mirroring fastchart_stock.c. */
$sma = function (int $i) use ($closes, $P): float {
    $s = 0.0; for ($k = $i - $P + 1; $k <= $i; $k++) $s += $closes[$k];
    return $s / $P;
};
$wma = function (int $i) use ($closes, $P): float {
    $sw = 0.0; for ($k = 0; $k < $P; $k++) $sw += ($k + 1) * $closes[$i - $P + 1 + $k];
    return $sw / ($P * ($P + 1) / 2);
};
$ema = [];
$seed = 0.0; for ($k = 0; $k < $P; $k++) $seed += $closes[$k];
$e = $seed / $P; $alpha = 2.0 / ($P + 1);
for ($i = $P - 1; $i < 40; $i++) { if ($i >= $P) $e = $alpha * $closes[$i] + (1 - $alpha) * $e; $ema[$i] = $e; }

$mk = fn() => (new FastChart\StockChart(600, 400))
    ->setOhlcv($rows)->setYAxisRange($MIN, $MAX)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);

$refs = ['SMA' => $sma, 'EMA' => fn($i) => $ema[$i], 'WMA' => $wma];
$types = ['SMA' => FastChart\StockChart::MA_SMA,
          'EMA' => FastChart\StockChart::MA_EMA,
          'WMA' => FastChart\StockChart::MA_WMA];
$ys22 = [];
foreach ($types as $name => $type) {
    $svg = $mk()->addMovingAverage($P, $type)->renderSvg();
    $pts = ov_points($svg);
    $pr  = plot_rect($svg);
    $maxdiff = 0;
    foreach ($pts as $m => $p) {
        $i = $P - 1 + $m;
        $expected = ypix($refs[$name]($i), $MIN, $MAX, $pr);
        $maxdiff = max($maxdiff, abs((int)$p[1] - $expected));
    }
    /* m = 13 is bar index 22 — three bars after the step. */
    $ys22[$name] = (int)$pts[13][1];
    echo "$name: npts=", count($pts), " maxdiff=$maxdiff\n";
}

/* Smaller y = higher value. Three bars into the 200-level the faster
 * EMA is nearer 200 (smaller y) than the slower SMA. */
echo "ema_faster_than_sma: ", ($ys22['EMA'] < $ys22['SMA'] ? 'yes' : 'NO'), "\n";
?>
--EXPECT--
SMA: npts=31 maxdiff=0
EMA: npts=31 maxdiff=0
WMA: npts=31 maxdiff=0
ema_faster_than_sma: yes
