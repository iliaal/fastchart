--TEST--
addOverlaySeries: combo charts (line over bar / line over area / etc.)
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

// Bar chart with a line overlay (the GDChart COMBO_LINE_BAR equivalent).
$bytes = (new FastChart\BarChart(800, 400))
    ->setSeries([10, 25, 18, 30, 22])
    ->addOverlaySeries('line', [12, 22, 20, 28, 24],
                       ['color' => 0xFF0000, 'thickness' => 3, 'label' => 'avg'])
    ->renderPng();
$im = imagecreatefromstring($bytes);

// The overlay line is explicit red. Sample for red pixels in the
// plot interior.
$red_hits = 0;
for ($y = 50; $y < 380; $y++) {
    for ($x = 60; $x < 760; $x++) {
        if (imagecolorat($im, $x, $y) === 0xFF0000) $red_hits++;
    }
}
echo "bar_with_line_overlay: ", ($red_hits > 100 ? "yes" : "no ($red_hits)"), "\n";

// Area overlay on a line chart. The area is filled translucent so
// it composites into intermediate shades.
$bytes2 = (new FastChart\LineChart(800, 400))
    ->setSeries([10, 30, 20, 40, 25])
    ->addOverlaySeries('area', [5, 20, 15, 30, 18],
                       ['color' => 0x008800])
    ->renderPng();
echo "line_with_area_overlay: ", substr(bin2hex($bytes2), 0, 8) === '89504e47' ? "ok" : "bad", "\n";

// Stock chart with overlay (time-axis variant).
$rows = [];
$ind  = [];
for ($i = 0; $i < 10; $i++) {
    $rows[] = [1700000000 + $i * 86400, 100 + $i, 102 + $i, 99 + $i, 101 + $i];
    $ind[]  = 50 + $i;
}
$bytes = (new FastChart\StockChart(900, 500))
    ->setOhlcv($rows)
    ->addOverlaySeries('line', $ind, ['color' => 0xFFCC00, 'thickness' => 2])
    ->renderPng();
echo "stock_with_overlay: ", strlen($bytes) > 2048 ? "ok" : "bad", "\n";

// Bad type rejected.
try {
    (new FastChart\BarChart)->addOverlaySeries('histogram', [1, 2, 3]);
    echo "bad_type: no throw\n";
} catch (\ValueError $e) {
    echo "bad_type: ValueError ok\n";
}

// Multiple overlays stack.
$bytes3 = (new FastChart\LineChart(800, 400))
    ->setSeries([1, 5, 3, 8, 4])
    ->addOverlaySeries('line', [2, 4, 4, 6, 5], ['color' => 0xFF0000])
    ->addOverlaySeries('line', [3, 3, 5, 5, 6], ['color' => 0x00CC00])
    ->renderPng();
$im3 = imagecreatefromstring($bytes3);
$has_red   = false;
$has_green = false;
for ($y = 30; $y < 380; $y++) {
    for ($x = 50; $x < 770; $x++) {
        $c = imagecolorat($im3, $x, $y);
        if ($c === 0xFF0000) $has_red = true;
        if ($c === 0x00CC00) $has_green = true;
        if ($has_red && $has_green) break 2;
    }
}
echo "two_overlays: ", ($has_red && $has_green ? "yes" : "no"), "\n";
?>
--EXPECT--
bar_with_line_overlay: yes
line_with_area_overlay: ok
stock_with_overlay: ok
bad_type: ValueError ok
two_overlays: yes
