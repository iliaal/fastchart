--TEST--
setBarWidth narrows or widens the bar fill within its slot
--EXTENSIONS--
fastchart
--FILE--
<?php

// Bar in default light-theme series 0 color #1f77b4. Sample the
// pixel coverage at a fixed band.
$count_blue = function ($im, $color, $w, $h) {
    $n = 0;
    for ($y = 0; $y < $h; $y++)
        for ($x = 0; $x < $w; $x++)
            if (imagecolorat($im, $x, $y) === $color) $n++;
    return $n;
};

$series_color = (0x1f << 16) | (0x77 << 8) | 0xb4;

$bytes_full = (new FastChart\BarChart(500, 400))
    ->setBarWidth(100)
    ->setSeries([10, 20, 30, 25, 15])
    ->renderPng();
$im_full = imagecreatefromstring($bytes_full);
$wide = $count_blue($im_full, $series_color, 500, 400);

$bytes_narrow = (new FastChart\BarChart(500, 400))
    ->setBarWidth(20)
    ->setSeries([10, 20, 30, 25, 15])
    ->renderPng();
$im_narrow = imagecreatefromstring($bytes_narrow);
$narrow = $count_blue($im_narrow, $series_color, 500, 400);

echo "wide_renders: ", ($wide > 100 ? "yes" : "no"), "\n";
echo "narrow_renders: ", ($narrow > 100 ? "yes" : "no"), "\n";
echo "narrow_lt_wide: ", ($narrow < $wide ? "yes" : "no"), "\n";

// Out-of-range rejected.
try {
    (new FastChart\BarChart)->setBarWidth(0);
    echo "zero: no throw\n";
} catch (\ValueError $e) {
    echo "zero: ValueError ok\n";
}
try {
    (new FastChart\BarChart)->setBarWidth(101);
    echo "over: no throw\n";
} catch (\ValueError $e) {
    echo "over: ValueError ok\n";
}
?>
--EXPECT--
wide_renders: yes
narrow_renders: yes
narrow_lt_wide: yes
zero: ValueError ok
over: ValueError ok
