--TEST--
StockChart: VWAP, ZigZag, ATR, CCI, Williams %R, StdDev, Aroon indicators
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* Each new indicator must render a well-formed, finite SVG (no overflow
 * sentinels), stack alongside others under the bumped caps, and Aroon must
 * emit two series. No gd / imagedestroy so this runs under UBSan. */
function clean(string $svg): bool {
    foreach (['-2147483648', 'NaN', 'nan', 'inf', '="-2', '="-1.#'] as $bad) {
        if (strpos($svg, $bad) !== false) return false;
    }
    return simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

/* 40 bars: upward drift plus an oscillation so pivots, ranges, and
 * volatility measures are all non-trivial. */
$ohlcv = [];
for ($i = 0; $i < 40; $i++) {
    $o = 100 + $i * 0.5 + 8 * sin($i / 3.0);
    $c = $o + cos($i / 2.0);
    $h = max($o, $c) + 1.5;
    $l = min($o, $c) - 1.5;
    $ohlcv[] = [1700000000 + $i * 86400, $o, $h, $l, $c, 1000 + ($i % 5) * 250];
}
$mk = fn() => (new FastChart\StockChart(600, 400))->setOhlcv($ohlcv);

$cases = [
    'vwap'      => fn($s) => $s->addVWAP(),
    'zigzag'    => fn($s) => $s->addZigZag(4.0),
    'atr'       => fn($s) => $s->addATR(14),
    'cci'       => fn($s) => $s->addCCI(20),
    'williamsr' => fn($s) => $s->addWilliamsR(14),
    'stddev'    => fn($s) => $s->addStdDev(20),
    'aroon'     => fn($s) => $s->addAroon(25),
];
foreach ($cases as $name => $add) {
    echo "$name: ", (clean($add($mk())->renderSvg()) ? "ok" : "BAD"), "\n";
}

/* Five indicators stacked (well under the 6/6 caps). */
$combo = $mk()->addVWAP()->addZigZag(5.0)->addATR()->addCCI()->addWilliamsR()->renderSvg();
echo "combo: ", (clean($combo) ? "ok" : "BAD"), "\n";

/* Aroon's two-series pane draws more lines than a single-series pane at
 * the same period. */
$aroon  = $mk()->addAroon(5)->renderSvg();
$single = $mk()->addATR(5)->renderSvg();
echo "aroon_two_series: ",
    (substr_count($aroon, '<line') > substr_count($single, '<line') ? "yes" : "no"), "\n";

/* No candles → throws. */
try {
    (new FastChart\StockChart(300, 200))->addATR();
    echo "no_candles: no_throw\n";
} catch (\Throwable $e) {
    echo "no_candles: threw\n";
}

echo "ok\n";
?>
--EXPECT--
vwap: ok
zigzag: ok
atr: ok
cci: ok
williamsr: ok
stddev: ok
aroon: ok
combo: ok
aroon_two_series: yes
no_candles: threw
ok
