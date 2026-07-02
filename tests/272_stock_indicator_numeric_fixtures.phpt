--TEST--
Stock indicators: hand-computable fixtures pin ATR/StdDev/CCI/Williams %R/Aroon values
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Coverage gap: indicator tests only asserted well-formed SVG, so a
 * broken formula that produced plausible finite numbers would pass.
 * These fixtures have hand-derivable values:
 *   - flat OHLC (O=H=L=C, constant): true range = 0 -> ATR = 0;
 *     stddev = 0; CCI mean deviation = 0 (division guard) -> constant.
 *   - strictly rising bars with close == high: Williams %R = 0 every
 *     bar (pane top); close == low gives -100 (pane bottom).
 *   - strictly rising highs and lows: Aroon Up = 100 (top line),
 *     Aroon Down = 0 (bottom line), both flat.
 * Indicator pane series are the only 2px lines in the document, so
 * they can be extracted by stroke-width. */

function pane_lines(string $svg): array {
    preg_match_all('/<line x1="([-0-9.]+)" y1="([-0-9.]+)" x2="([-0-9.]+)" y2="([-0-9.]+)"[^>]*stroke-width="2"/',
        $svg, $m, PREG_SET_ORDER);
    return $m;
}

function distinct_ys(array $lines): array {
    $ys = [];
    foreach ($lines as $l) { $ys[$l[2]] = true; $ys[$l[4]] = true; }
    $ys = array_map('floatval', array_keys($ys));
    sort($ys);
    return $ys;
}

$flat = [];
for ($i = 0; $i < 30; $i++) {
    $flat[] = [1700000000 + $i * 86400, 100, 100, 100, 100, 1000];
}
$rise_hi = $rise_lo = [];
for ($i = 0; $i < 30; $i++) {
    $l = 100 + $i;
    $h = $l + 2;
    $rise_hi[] = [1700000000 + $i * 86400, $l + 1, $h, $l, $h, 1000];  /* close == high */
    $rise_lo[] = [1700000000 + $i * 86400, $l + 1, $h, $l, $l, 1000];  /* close == low  */
}
$mk = fn(array $ohlcv) => (new FastChart\StockChart(600, 400))->setOhlcv($ohlcv);

/* Zero-volatility input: each indicator must be constant (one flat
 * line) and emit no NaN — CCI divides by mean deviation (0 here). */
foreach (['addATR' => 'atr', 'addStdDev' => 'stddev', 'addCCI' => 'cci'] as $meth => $name) {
    $svg = $mk($flat)->$meth(5)->renderSvg();
    $lines = pane_lines($svg);
    $flat_ok = count($lines) > 10 && count(distinct_ys($lines)) === 1;
    $clean   = stripos($svg, 'nan') === false && stripos($svg, 'inf') === false;
    echo "$name: ", ($flat_ok ? 'constant' : 'NOT CONSTANT'),
         " ", ($clean ? 'finite' : 'NON-FINITE'), "\n";
}

/* Williams %R: close==high -> 0 (top of the [-100, 0] pane);
 * close==low -> -100 (bottom). Both constant; top strictly above. */
$top = distinct_ys(pane_lines($mk($rise_hi)->addWilliamsR(5)->renderSvg()));
$bot = distinct_ys(pane_lines($mk($rise_lo)->addWilliamsR(5)->renderSvg()));
echo "wr_high_constant: ", (count($top) === 1 ? 'yes' : 'NO'), "\n";
echo "wr_low_constant: ",  (count($bot) === 1 ? 'yes' : 'NO'), "\n";
echo "wr_high_above_low: ", ($top[0] < $bot[0] ? 'yes' : 'NO'), "\n";

/* Aroon on strictly rising data: exactly two flat lines, Up (100) at
 * the pane top and Down (0) at the bottom. */
$ys = distinct_ys(pane_lines($mk($rise_hi)->addAroon(5)->renderSvg()));
echo "aroon_two_flat_lines: ", (count($ys) === 2 ? 'yes' : 'NO (' . count($ys) . ')'), "\n";
echo "aroon_up_above_down: ", (count($ys) === 2 && $ys[0] < $ys[1] ? 'yes' : 'NO'), "\n";

/* Williams %R = 0 and Aroon Up = 100 are each the max of their pane
 * range, so both must map to the same pane-top pixel row. */
echo "pane_top_agrees: ", (abs($top[0] - $ys[0]) < 0.001 ? 'yes' : 'NO'), "\n";

?>
--EXPECT--
atr: constant finite
stddev: constant finite
cci: constant finite
wr_high_constant: yes
wr_low_constant: yes
wr_high_above_low: yes
aroon_two_flat_lines: yes
aroon_up_above_down: yes
pane_top_agrees: yes
