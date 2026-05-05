--TEST--
StockChart::setCandleStyle: STYLE_HOLLOW / STYLE_VOLUME / STYLE_VECTOR
--EXTENSIONS--
fastchart
--FILE--
<?php

// Synthetic OHLCV: alternating up/down bars with volume spikes every
// few bars so the vector / volume styles have something to react to.
$rows = [];
for ($i = 0; $i < 30; $i++) {
    $base = 100 + sin($i / 3) * 5;
    $vol  = 1000 * (1 + ($i % 7 == 0 ? 3.0 : ($i % 5 == 0 ? 1.5 : 0.5)));
    $rows[] = [
        1700000000 + $i * 86400,
        $base,
        $base + 2,
        $base - 2,
        $base + (($i % 2) ? 1 : -1),
        $vol,
    ];
}

// All three new styles render valid PNGs.
foreach ([
    'HOLLOW' => FastChart\Chart::STYLE_HOLLOW,
    'VOLUME' => FastChart\Chart::STYLE_VOLUME,
    'VECTOR' => FastChart\Chart::STYLE_VECTOR,
] as $name => $style) {
    $b = (new FastChart\StockChart(700, 400))
        ->setOhlcv($rows)
        ->setCandleStyle($style)
        ->renderPng();
    echo "$name: ", strlen($b) > 1024 ? "ok" : "bad", "\n";
}

// VECTOR mode injects four distinct strength colors (lime, cyan,
// red, fuchsia) when both directions and strengths are present in
// the data. Verify at least the bullish-strong (lime ~ 0x00E640)
// shows up: the spike at $i=0 is a bullish bar with volume above
// avg.
$bytes = (new FastChart\StockChart(900, 500))
    ->setOhlcv($rows)
    ->setCandleStyle(FastChart\Chart::STYLE_VECTOR)
    ->renderPng();
$im = imagecreatefromstring($bytes);
$has = function ($im, $c, $w, $h) {
    for ($y = 0; $y < $h; $y++)
        for ($x = 0; $x < $w; $x++)
            if (imagecolorat($im, $x, $y) === $c) return true;
    return false;
};
echo "vector_lime: ",     $has($im, 0x00E640, 900, 500) ? "yes" : "no", "\n";
echo "vector_red: ",      $has($im, 0xE30000, 900, 500) ? "yes" : "no", "\n";

// HOLLOW differs visually from CANDLE (bullish bodies are outline-only).
$candle = (new FastChart\StockChart(700, 400))
    ->setOhlcv($rows)
    ->setCandleStyle(FastChart\Chart::STYLE_CANDLE)
    ->renderPng();
$hollow = (new FastChart\StockChart(700, 400))
    ->setOhlcv($rows)
    ->setCandleStyle(FastChart\Chart::STYLE_HOLLOW)
    ->renderPng();
echo "hollow_differs: ", ($candle !== $hollow ? "yes" : "no"), "\n";

// VOLUME differs visually from CANDLE (varying body widths).
$volume = (new FastChart\StockChart(700, 400))
    ->setOhlcv($rows)
    ->setCandleStyle(FastChart\Chart::STYLE_VOLUME)
    ->renderPng();
echo "volume_differs: ", ($candle !== $volume ? "yes" : "no"), "\n";

// Out-of-range still rejected.
try {
    (new FastChart\StockChart)->setCandleStyle(99);
    echo "bad: no throw\n";
} catch (\ValueError $e) {
    echo "bad: ValueError ok\n";
}
?>
--EXPECT--
HOLLOW: ok
VOLUME: ok
VECTOR: ok
vector_lime: yes
vector_red: yes
hollow_differs: yes
volume_differs: yes
bad: ValueError ok
