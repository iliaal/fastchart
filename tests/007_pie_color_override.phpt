--TEST--
PieChart: per-slice color override via 'color' key
--SKIPIF--
<?php if (!extension_loaded("fastchart")) print "skip fastchart not loaded"; ?>
--FILE--
<?php

$im = imagecreatetruecolor(600, 600);

// All three slices request explicit colors that are NOT in the
// default light palette. If override is honored, the canvas
// contains 0xff00ff / 0x00ffff / 0xffff00 pixels in the pie region
// and NOT the default series[0..2] colors.
(new FastChart\PieChart())
    ->setSize(600, 600)
    ->setSlices([
        ['label' => 'A', 'value' => 30, 'color' => 0xFF00FF],  // magenta
        ['label' => 'B', 'value' => 30, 'color' => 0x00FFFF],  // cyan
        ['label' => 'C', 'value' => 30, 'color' => 0xFFFF00],  // yellow
    ])
    ->draw($im);

$expected = [
    'magenta' => 0xff00ff,
    'cyan'    => 0x00ffff,
    'yellow'  => 0xffff00,
];
$default_blue = 0x1f77b4;

$found = ['magenta' => false, 'cyan' => false, 'yellow' => false];
$default_seen = false;
for ($y = 80; $y < 540; $y += 2) {
    for ($x = 60; $x < 540; $x += 2) {
        $c = imagecolorat($im, $x, $y);
        if ($c === $default_blue) $default_seen = true;
        foreach ($expected as $name => $want) {
            if ($c === $want) $found[$name] = true;
        }
    }
}
echo "magenta_present: ", $found['magenta'] ? "yes" : "no", "\n";
echo "cyan_present: ",    $found['cyan']    ? "yes" : "no", "\n";
echo "yellow_present: ",  $found['yellow']  ? "yes" : "no", "\n";
echo "default_palette_used: ", $default_seen ? "yes" : "no", "\n";

// Out-of-range color values fall back to the palette without error.
$im2 = imagecreatetruecolor(600, 600);
$out = (new FastChart\PieChart())
    ->setSize(600, 600)
    ->setSlices([
        ['label' => 'A', 'value' => 50, 'color' => -1],          // invalid
        ['label' => 'B', 'value' => 50, 'color' => 0x1000000],   // > 24-bit
    ])
    ->draw($im2);
var_dump($out instanceof \GdImage);
?>
--EXPECT--
magenta_present: yes
cyan_present: yes
yellow_present: yes
default_palette_used: no
bool(true)
