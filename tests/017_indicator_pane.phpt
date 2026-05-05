--TEST--
StockChart::addIndicatorPane stacks indicator sub-panes below price (and volume)
--EXTENSIONS--
fastchart
--FILE--
<?php

$rows = [];
$rsi = [];
$macd = [];
$base = 1700000000;
for ($i = 0; $i < 30; $i++) {
    $p = 100 + sin($i / 4) * 5;
    $rows[] = [$base + $i * 86400, $p, $p + 0.5, $p - 0.5, $p + 0.2, 1000 + $i * 50];
    $rsi[]  = 30 + ($i * 3 % 50);    // 30..80
    $macd[] = sin($i / 6) * 2;        // -2..2
}

// Single indicator pane, with reference line at 50 (RSI midline).
$im = imagecreatetruecolor(1000, 600);
$out = (new FastChart\StockChart)
    ->setSize(1000, 600)
    ->setOhlcv($rows)
    ->addIndicatorPane('RSI', $rsi, [
        'color' => 0x884488,
        'reference' => 50.0,
        'min' => 0.0,
        'max' => 100.0,
    ])
    ->draw($im);
var_dump($out instanceof \GdImage);

// The pane color (0x884488) should appear in the bottom region of
// the canvas (below the price pane).
$pane_color = 0x884488;
$pane_hits = 0;
for ($y = 420; $y < 560; $y++) {
    for ($x = 60; $x < 940; $x++) {
        if (imagecolorat($im, $x, $y) === $pane_color) $pane_hits++;
    }
}
echo "rsi_pane_line_visible: ", ($pane_hits > 30 ? "yes" : "no ($pane_hits)"), "\n";

// Two indicator panes stack below price. Volume + 2 indicators is
// also valid: 3 sub-panes total.
$im2 = imagecreatetruecolor(1000, 700);
$out = (new FastChart\StockChart)
    ->setSize(1000, 700)
    ->setOhlcv($rows)
    ->setVolumePane(true)
    ->addIndicatorPane('RSI',  $rsi,  ['min' => 0.0, 'max' => 100.0])
    ->addIndicatorPane('MACD', $macd)
    ->draw($im2);
var_dump($out instanceof \GdImage);

// Cap at 3 panes -- a 4th raises ValueError.
$builder = (new FastChart\StockChart)
    ->setOhlcv($rows)
    ->addIndicatorPane('p1', $rsi)
    ->addIndicatorPane('p2', $rsi)
    ->addIndicatorPane('p3', $rsi);
try {
    $builder->addIndicatorPane('p4', $rsi);
    echo "fourth_pane: no throw\n";
} catch (\ValueError $e) {
    echo "fourth_pane: ValueError ok\n";
}

// Empty name rejected.
try {
    (new FastChart\StockChart)->addIndicatorPane('', $rsi);
    echo "empty_name: no throw\n";
} catch (\ValueError $e) {
    echo "empty_name: ValueError ok\n";
}
?>
--EXPECT--
bool(true)
rsi_pane_line_visible: yes
bool(true)
fourth_pane: ValueError ok
empty_name: ValueError ok
