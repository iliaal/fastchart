--TEST--
AreaChart: stacked + non-stacked overlapping fills
--EXTENSIONS--
fastchart
--FILE--
<?php

// Stacked: cumulative fills, opaque.
$im = imagecreatetruecolor(900, 500);
$out = (new FastChart\AreaChart)
    ->setSize(900, 500)
    ->setSeries([
        ['label' => 'A', 'data' => [10, 20, 15, 25, 30]],
        ['label' => 'B', 'data' => [5,  10, 12,  8, 15]],
        ['label' => 'C', 'data' => [3,   4,  6,  8,  5]],
    ])
    ->setStacked(true)
    ->draw($im);
var_dump($out instanceof \GdImage);

// Sample for the three series colors inside the plot area.
$colors = [
    'blue'   => 0x1f77b4,
    'orange' => 0xff7f0e,
    'green'  => 0x2ca02c,
];
$found = ['blue' => false, 'orange' => false, 'green' => false];
for ($y = 50; $y < 470; $y += 4) {
    for ($x = 80; $x < 870; $x += 4) {
        $c = imagecolorat($im, $x, $y);
        foreach ($colors as $name => $want) {
            if ($c === $want) $found[$name] = true;
        }
    }
}
echo "stacked_three_series_visible: ",
    ($found['blue'] && $found['orange'] && $found['green'] ? "yes" : "no"), "\n";

// Non-stacked overlapping: alpha-blended fills.
$im2 = imagecreatetruecolor(900, 500);
(new FastChart\AreaChart)
    ->setSize(900, 500)
    ->setSeries([
        ['label' => 'A', 'data' => [30, 20, 25, 15, 10]],
        ['label' => 'B', 'data' => [10, 25, 15, 25, 20]],
    ])
    ->setStacked(false)
    ->setFillOpacity(80)
    ->draw($im2);

// Renderable PNG output.
ob_start(); imagepng($im2); $png = ob_get_clean();
echo "non_stacked_png_sig: ", substr(bin2hex($png), 0, 8) === '89504e47' ? "ok" : "bad", "\n";
echo "non_stacked_size_sane: ", (strlen($png) > 1024 ? "yes" : "no"), "\n";

// AreaChart honors setMissing / null sentinel for gaps.
$im3 = imagecreatetruecolor(900, 500);
$out = (new FastChart\AreaChart)
    ->setSize(900, 500)
    ->setSeries([1, 2, null, 4, 5])
    ->draw($im3);
var_dump($out instanceof \GdImage);

// Bad opacity rejected.
try {
    (new FastChart\AreaChart)->setFillOpacity(-1);
    echo "opacity_neg: no throw\n";
} catch (\ValueError $e) {
    echo "opacity_neg: ValueError ok\n";
}

try {
    (new FastChart\AreaChart)->setFillOpacity(200);
    echo "opacity_200: no throw\n";
} catch (\ValueError $e) {
    echo "opacity_200: ValueError ok\n";
}
?>
--EXPECT--
bool(true)
stacked_three_series_visible: yes
non_stacked_png_sig: ok
non_stacked_size_sane: yes
bool(true)
opacity_neg: ValueError ok
opacity_200: ValueError ok
