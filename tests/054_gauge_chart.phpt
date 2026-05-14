--TEST--
GaugeChart: dial readout with zones, needle, and value label
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

// Single-color gauge (no zones).
$bytes = (new FastChart\GaugeChart(400, 300))
    ->setRange(0, 200)
    ->setValue(140)
    ->setTitle('Speed')
    ->renderPng();
echo "renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

// Three colored zones.
$bytes2 = (new FastChart\GaugeChart(400, 300))
    ->setRange(0, 200)
    ->setValue(140)
    ->setZones([
        ['from' => 0,   'to' => 80,  'color' => 0x66cc66],
        ['from' => 80,  'to' => 140, 'color' => 0xddcc44],
        ['from' => 140, 'to' => 200, 'color' => 0xcc4444],
    ])
    ->renderPng();
$im = imagecreatefromstring($bytes2);

$has = function ($im, $c, $w, $h) {
    for ($y = 0; $y < $h; $y++)
        for ($x = 0; $x < $w; $x++)
            if (imagecolorat($im, $x, $y) === $c) return true;
    return false;
};
echo "green_zone: ",  $has($im, 0x66cc66, 400, 300) ? "yes" : "no", "\n";
echo "yellow_zone: ", $has($im, 0xddcc44, 400, 300) ? "yes" : "no", "\n";
echo "red_zone: ",    $has($im, 0xcc4444, 400, 300) ? "yes" : "no", "\n";

// Value format string.
$bytes3 = (new FastChart\GaugeChart(400, 300))
    ->setRange(0, 100)
    ->setValue(62.5)
    ->setValueFormat('%.0f%%')
    ->renderPng();
echo "format: ", strlen($bytes3) > 1024 ? "ok" : "bad", "\n";

// Bad range.
try {
    (new FastChart\GaugeChart)->setRange(100, 0);
    echo "bad_range: no throw\n";
} catch (\ValueError $e) {
    echo "bad_range: ValueError ok\n";
}
?>
--EXPECT--
renders: ok
green_zone: yes
yellow_zone: yes
red_zone: yes
format: ok
bad_range: ValueError ok
