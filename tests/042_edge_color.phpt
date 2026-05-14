--TEST--
setEdgeColor draws an outline around bars / area fills / pie slices
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

// Helper added by plutovg pixel-tolerance sweep: accepts $pixel as an
// AA-blended version of $target against a white background. Plutovg's
// 1px strokes produce ~50%-coverage centerline pixels rather than the
// pure target color libgd emitted.
function fc_color_near($pixel, $target) {
    $tr = ($target >> 16) & 0xFF;
    $tg = ($target >>  8) & 0xFF;
    $tb =  $target        & 0xFF;
    $r = ($pixel >> 16) & 0xFF;
    $g = ($pixel >>  8) & 0xFF;
    $b =  $pixel        & 0xFF;
    for ($a = 100; $a >= 30; $a -= 5) {
        $alpha = $a / 100.0;
        $er = (int)($tr * $alpha + 255 * (1 - $alpha));
        $eg = (int)($tg * $alpha + 255 * (1 - $alpha));
        $eb = (int)($tb * $alpha + 255 * (1 - $alpha));
        if (abs($r - $er) <= 4 && abs($g - $eg) <= 4 && abs($b - $eb) <= 4) {
            return true;
        }
    }
    return false;
}


// Bar with explicit edge color black.
$bytes = (new FastChart\BarChart(500, 400))
    ->setEdgeColor(0x000000)
    ->setSeries([10, 20, 30, 25])
    ->renderPng();
$im = imagecreatefromstring($bytes);

// Black pixels along bar outlines should appear inside the plot.
$black = 0;
for ($y = 0; $y < 400; $y++)
    for ($x = 0; $x < 500; $x++)
        if (fc_color_near(imagecolorat($im, $x, $y), 0x000000)) $black++;
echo "bar_edge_present: ", ($black > 100 ? "yes" : "no"), "\n";

// AreaChart edge.
$bytes2 = (new FastChart\AreaChart(500, 400))
    ->setEdgeColor(0xFF0000)
    ->setStacked(false)
    ->setSeries([1, 5, 3, 8, 4])
    ->renderPng();
$im2 = imagecreatefromstring($bytes2);
$red = 0;
for ($y = 0; $y < 400; $y++)
    for ($x = 0; $x < 500; $x++)
        if (fc_color_near(imagecolorat($im2, $x, $y), 0xFF0000)) $red++;
echo "area_edge_present: ", ($red > 100 ? "yes" : "no"), "\n";

// Pie edge: green outline replaces theme border.
$bytes3 = (new FastChart\PieChart(400, 400))
    ->setEdgeColor(0x00FF00)
    ->setSlices(['A' => 40, 'B' => 30, 'C' => 30])
    ->renderPng();
$im3 = imagecreatefromstring($bytes3);
$green = 0;
for ($y = 0; $y < 400; $y++)
    for ($x = 0; $x < 400; $x++)
        if (fc_color_near(imagecolorat($im3, $x, $y), 0x00FF00)) $green++;
echo "pie_edge_present: ", ($green > 100 ? "yes" : "no"), "\n";

// -1 disables.
$bytes4 = (new FastChart\BarChart(500, 400))
    ->setEdgeColor(0x000000)
    ->setEdgeColor(-1)
    ->setSeries([10, 20, 30])
    ->renderPng();
echo "disable_renders: ", strlen($bytes4) > 1024 ? "ok" : "bad", "\n";

// Out-of-range.
try {
    (new FastChart\BarChart)->setEdgeColor(0x1000000);
    echo "bad: no throw\n";
} catch (\ValueError $e) {
    echo "bad: ValueError ok\n";
}
?>
--EXPECT--
bar_edge_present: yes
area_edge_present: yes
pie_edge_present: yes
disable_renders: ok
bad: ValueError ok
