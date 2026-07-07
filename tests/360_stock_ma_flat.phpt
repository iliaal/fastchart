--TEST--
StockChart moving averages: SMA/EMA/WMA of a constant close render as a flat overlay
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Coverage gap: MA_EMA and MA_WMA were never exercised. With a
 * constant close every average (SMA, EMA seeded with the leading
 * SMA, WMA) equals that close on every bar, so each 2px overlay must
 * be a single horizontal line: all its points share one y, and that
 * y is the pixel of the constant close. The MA overlay is the only
 * 2px line a StockChart emits (candle wicks are 1px, bodies are
 * rects), so it extracts cleanly by stroke-width. */

/* Extract the ordered overlay points from the 2px line segments. */
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

/* Plot rect = second white rect; y_to_pixel replicated from the axis. */
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

$MIN = 90.0; $MAX = 110.0;
$rows = [];
for ($i = 0; $i < 30; $i++) {
    $rows[] = [1700000000 + $i * 86400, 100.0, 105.0, 95.0, 100.0, 1000];
}
$mk = fn() => (new FastChart\StockChart(600, 400))
    ->setOhlcv($rows)->setYAxisRange($MIN, $MAX)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);

foreach (['SMA' => FastChart\StockChart::MA_SMA,
          'EMA' => FastChart\StockChart::MA_EMA,
          'WMA' => FastChart\StockChart::MA_WMA] as $name => $type) {
    $svg = $mk()->addMovingAverage(5, $type)->renderSvg();
    $pts = ov_points($svg);
    $pr  = plot_rect($svg);
    $ys  = array_unique(array_column($pts, 1));
    $flat = count($pts) > 10 && count($ys) === 1;
    $on_close = count($ys) === 1 && (int)reset($ys) === ypix(100.0, $MIN, $MAX, $pr);
    $clean = stripos($svg, 'nan') === false && stripos($svg, 'inf') === false;
    echo "$name: ", ($flat ? 'flat' : 'NOT FLAT'),
         " ", ($on_close ? 'at_close' : 'OFF'),
         " ", ($clean ? 'finite' : 'NON-FINITE'), "\n";
}
?>
--EXPECT--
SMA: flat at_close finite
EMA: flat at_close finite
WMA: flat at_close finite
