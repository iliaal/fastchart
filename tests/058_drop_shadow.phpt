--TEST--
setDropShadow paints offset shadow underneath bars / pies / areas / text
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

// Drop shadow paints a translucent dark fill at the offset position,
// so the pixel histogram of the shadowed render gains many darker
// (lower-luma) pixels around each bar. Compare the count of "dark
// non-bar" pixels: any pixel under luma 200 that's neither the
// series color nor pure white. Using the bytes-differ test is the
// most stable signal across libgd alpha-blending behavior.
$bytes_off = (new FastChart\BarChart(500, 400))
    ->setSeries([10, 20, 30])
    ->renderPng();
$bytes_on = (new FastChart\BarChart(500, 400))
    ->setDropShadow(5, 5, 0x000000)
    ->setSeries([10, 20, 30])
    ->renderPng();

echo "shadow_changes_render: ", ($bytes_on !== $bytes_off ? "yes" : "no"), "\n";

// Sample the offset region to the lower-right of where a bar would
// be. Without shadow it's plain bg/grid; with shadow it picks up a
// gray fill. Count "darkish" pixels (luma < 200) that aren't the
// series color.
$im_on = imagecreatefromstring($bytes_on);
$im_off = imagecreatefromstring($bytes_off);
$series_color = (0x1f << 16) | (0x77 << 8) | 0xb4;
$count_dark_extra = function ($im, $series, $w, $h) {
    $n = 0;
    for ($y = 0; $y < $h; $y++) {
        for ($x = 0; $x < $w; $x++) {
            $c = imagecolorat($im, $x, $y);
            if ($c === $series) continue;
            $r = ($c >> 16) & 0xFF;
            $g = ($c >>  8) & 0xFF;
            $b =  $c        & 0xFF;
            $luma = ($r * 299 + $g * 587 + $b * 114) / 1000;
            if ($luma < 200) $n++;
        }
    }
    return $n;
};
$on_dark  = $count_dark_extra($im_on,  $series_color, 500, 400);
$off_dark = $count_dark_extra($im_off, $series_color, 500, 400);
echo "shadow_adds_dark: ", ($on_dark > $off_dark ? "yes" : "no ($off_dark vs $on_dark)"), "\n";

// Shadow on pie.
$bytes_pie = (new FastChart\PieChart(400, 400))
    ->setSlices(['A' => 40, 'B' => 35, 'C' => 25])
    ->setDropShadow(4, 4, 0x000000)
    ->renderPng();
echo "pie_shadow: ", strlen($bytes_pie) > 1024 ? "ok" : "bad", "\n";

// Disable.
$bytes_dis = (new FastChart\BarChart(400, 300))
    ->setDropShadow(3, 3)
    ->setDropShadow(0, 0)
    ->setSeries([10, 20])
    ->renderPng();
echo "disable_renders: ", strlen($bytes_dis) > 1024 ? "ok" : "bad", "\n";

// Out-of-range offset.
try {
    (new FastChart\BarChart)->setDropShadow(100, 0);
    echo "bad: no throw\n";
} catch (\ValueError $e) {
    echo "bad: ValueError ok\n";
}
?>
--EXPECT--
shadow_changes_render: yes
shadow_adds_dark: yes
pie_shadow: ok
disable_renders: ok
bad: ValueError ok
