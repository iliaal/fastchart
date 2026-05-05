--TEST--
StockChart::setVolumeColors per-bar override
--EXTENSIONS--
fastchart
--FILE--
<?php

$rows = [];
for ($i = 0; $i < 5; $i++) {
    $rows[] = [1700000000 + $i * 86400, 100, 102, 99, 101, 1000];
}

$colors = [0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xFF00FF];

$bytes = (new FastChart\StockChart(900, 500))
    ->setOhlcv($rows)
    ->setVolumePane(true)
    ->setVolumeColors($colors)
    ->renderPng();
$im = imagecreatefromstring($bytes);

// Each volume bar should have its assigned color visible somewhere
// in the volume pane (lower portion of the canvas).
$found = [];
for ($y = 350; $y < 500; $y++) {
    for ($x = 0; $x < 900; $x++) {
        $c = imagecolorat($im, $x, $y);
        foreach ($colors as $i => $want) {
            if ($c === $want) $found[$i] = true;
        }
    }
}
$count = count($found);
echo "all_5_volume_colors_present: ", ($count === 5 ? "yes" : "no ($count/5)"), "\n";

// Empty array reverts to default coloring.
$out = (new FastChart\StockChart(900, 500))
    ->setOhlcv($rows)
    ->setVolumePane(true)
    ->setVolumeColors($colors)
    ->setVolumeColors([])
    ->renderPng();
echo "empty_reverts_renders: ", strlen($out) > 1024 ? "ok" : "bad", "\n";
?>
--EXPECT--
all_5_volume_colors_present: yes
empty_reverts_renders: ok
