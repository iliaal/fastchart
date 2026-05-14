--TEST--
setPlotRect bypasses auto-layout; setBorderSides selects which sides draw
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


// Hard plot rectangle: chart draws with the user-specified bounds.
$bytes = (new FastChart\LineChart(800, 500))
    ->setPlotRect(100, 100, 700, 400)
    ->setSeries([1, 5, 3, 8])
    ->renderPng();
$im = imagecreatefromstring($bytes);

// Forced plot rect: top/left/bottom borders are overdrawn by grid
// lines and the X/Y axis lines, so we sample the right border
// (vertical, at x=700) which survives in border color (#666666).
$border = 0x666666;
$found_right = 0;
for ($y = 100; $y <= 400; $y++) {
    if (fc_color_near(imagecolorat($im, 700, $y), $border)) $found_right++;
}
echo "hard_rect_right_border: ", ($found_right > 200 ? "yes" : "no"), "\n";

// X-axis line in axis color sits at y=400 (the bottom of the
// forced rect); confirms vertical placement of the rect.
$axis = 0x333333;
$found_xaxis = 0;
for ($x = 100; $x <= 700; $x++) {
    if (fc_color_near(imagecolorat($im, $x, 400), $axis)) $found_xaxis++;
}
echo "hard_rect_xaxis: ", ($found_xaxis > 200 ? "yes" : "no"), "\n";

// Negative width reverts to auto-layout.
$bytes = (new FastChart\LineChart(400, 300))
    ->setPlotRect(50, 50, 100, 100)   // tiny rect first
    ->setPlotRect(0, 0, -1, -1)        // revert
    ->setSeries([1, 2, 3])
    ->renderPng();
echo "revert_renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

// BORDER_NONE: top/bottom/right border lines absent.
$bytes2 = (new FastChart\LineChart(400, 300))
    ->setBorderSides(FastChart\Chart::BORDER_NONE)
    ->setSeries([1, 5, 3, 8])
    ->renderPng();
$im2 = imagecreatefromstring($bytes2);

// Top edge of plot area should NOT have a border line
// (left edge has the Y-axis line which is separate).
$top_border_pixels = 0;
for ($x = 60; $x < 380; $x++) {
    /* Sample at the y-coord just inside the plot top -- if BORDER_TOP
     * was drawn, the border color would dominate. */
    if (fc_color_near(imagecolorat($im2, $x, 50), $border)) $top_border_pixels++;
}
echo "border_none_no_top: ", ($top_border_pixels < 5 ? "yes" : "no ($top_border_pixels)"), "\n";

// BORDER_LEFT only: left side has border, top/bottom don't.
$bytes3 = (new FastChart\LineChart(400, 300))
    ->setBorderSides(FastChart\Chart::BORDER_LEFT)
    ->setSeries([1, 5, 3, 8])
    ->renderPng();
echo "left_only_renders: ", strlen($bytes3) > 1024 ? "ok" : "bad", "\n";

// Out-of-range bitmask rejected.
try {
    (new FastChart\LineChart)->setBorderSides(255);
    echo "bad_sides: no throw\n";
} catch (\ValueError $e) {
    echo "bad_sides: ValueError ok\n";
}
?>
--EXPECT--
hard_rect_right_border: yes
hard_rect_xaxis: yes
revert_renders: ok
border_none_no_top: yes
left_only_renders: ok
bad_sides: ValueError ok
