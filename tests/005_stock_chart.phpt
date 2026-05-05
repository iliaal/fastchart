--TEST--
StockChart::draw renders candles, SMA overlay, optional volume pane
--EXTENSIONS--
fastchart
--FILE--
<?php

// 10 days of fake OHLCV starting 2023-11-15.
$rows = [];
$base = 1700000000;
$prices = [
    [100, 110, 95, 108],   // up day
    [108, 112, 102, 105],  // down day
    [105, 115, 100, 113],  // up day
    [113, 118, 110, 111],  // down day
    [111, 119, 108, 117],  // up day
    [117, 121, 114, 116],  // down day
    [116, 120, 112, 119],  // up day
    [119, 122, 116, 117],  // down day
    [117, 124, 115, 122],  // up day
    [122, 126, 119, 125],  // up day
];
foreach ($prices as $i => $p) {
    $rows[] = [$base + $i * 86400, $p[0], $p[1], $p[2], $p[3], 1000 + $i * 100];
}

$im = imagecreatetruecolor(1000, 600);
(new FastChart\StockChart())
    ->setSize(1000, 600)
    ->setTitle('AAPL test')
    ->setOhlcv($rows)
    ->setMovingAverages([3])
    ->setVolumePane(true)
    ->draw($im);

// Light theme: up candle = #2ca02c (green), down = #d62728 (red).
$up   = (0x2c << 16) | (0xa0 << 8) | 0x2c;
$down = (0xd6 << 16) | (0x27 << 8) | 0x28;

$found_up = $found_down = false;
for ($y = 60; $y < 540; $y += 2) {
    for ($x = 60; $x < 940; $x += 2) {
        $c = imagecolorat($im, $x, $y);
        if ($c === $up)   $found_up   = true;
        if ($c === $down) $found_down = true;
        if ($found_up && $found_down) break 2;
    }
}
echo "up_candles_visible: ",   ($found_up   ? "yes" : "no"), "\n";
echo "down_candles_visible: ", ($found_down ? "yes" : "no"), "\n";

// SMA overlay uses series[0] color (#1f77b4) on light theme.
$sma_color = (0x1f << 16) | (0x77 << 8) | 0xb4;
$found_sma = false;
for ($y = 60; $y < 480; $y += 2) {
    for ($x = 60; $x < 940; $x += 2) {
        if (imagecolorat($im, $x, $y) === $sma_color) {
            $found_sma = true;
            break 2;
        }
    }
}
echo "sma_overlay_visible: ", ($found_sma ? "yes" : "no"), "\n";

ob_start(); imagepng($im); $png = ob_get_clean();
echo "png_sig: ", substr(bin2hex($png), 0, 8) === '89504e47' ? "ok" : "bad", "\n";
echo "png_size>2k: ", (strlen($png) > 2048 ? "yes" : "no"), "\n";

// Same data without volume pane should still render the candles.
$im2 = imagecreatetruecolor(1000, 600);
(new FastChart\StockChart())
    ->setSize(1000, 600)
    ->setOhlcv($rows)
    ->draw($im2);
$found_up2 = false;
for ($y = 60; $y < 540; $y += 2) {
    for ($x = 60; $x < 940; $x += 2) {
        if (imagecolorat($im2, $x, $y) === $up) {
            $found_up2 = true;
            break 2;
        }
    }
}
echo "no_volume_pane_still_renders: ", ($found_up2 ? "yes" : "no"), "\n";

// Empty data must throw, not silently produce an empty plot.
try {
    (new FastChart\StockChart())->setOhlcv([])->draw(imagecreatetruecolor(100, 100));
    echo "empty_data_throws: no\n";
} catch (Error $e) {
    echo "empty_data_throws: yes\n";
}
?>
--EXPECT--
up_candles_visible: yes
down_candles_visible: yes
sma_overlay_visible: yes
png_sig: ok
png_size>2k: yes
no_volume_pane_still_renders: yes
empty_data_throws: yes
