--TEST--
setZeroShelf draws a horizontal axis-color line at y=0 when range crosses zero
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


// Data crosses zero. Without zero shelf, only the regular y=0 grid
// line in grid color appears; with shelf on, an axis-color line
// also runs across at the zero pixel.
$bytes_off = (new FastChart\LineChart(500, 400))
    ->setSeries([-5, -2, 1, 4, -1, 3])
    ->renderPng();
$im_off = imagecreatefromstring($bytes_off);

$bytes_on = (new FastChart\LineChart(500, 400))
    ->setZeroShelf(true)
    ->setSeries([-5, -2, 1, 4, -1, 3])
    ->renderPng();
$im_on = imagecreatefromstring($bytes_on);

// Look for any horizontal axis-color (#333333) stretch >= 200 px
// inside the plot interior. With shelf on, exactly the zero row
// satisfies this in axis color.
$has_h_axis_run = function ($im, $w, $h) {
    for ($y = 50; $y < $h - 60; $y++) {
        $n = 0;
        for ($x = 60; $x < $w - 30; $x++) {
            if (fc_color_near(imagecolorat($im, $x, $y), 0x333333)) $n++;
        }
        if ($n > 200) return true;
    }
    return false;
};

echo "off_no_shelf: ", $has_h_axis_run($im_off, 500, 400) ? "yes(unexpected)" : "no", "\n";
echo "on_has_shelf: ", $has_h_axis_run($im_on, 500, 400) ? "yes" : "no", "\n";

// Range that doesn't cross zero -- shelf is suppressed.
$bytes_pos = (new FastChart\LineChart(500, 400))
    ->setZeroShelf(true)
    ->setSeries([10, 20, 30])
    ->renderPng();
echo "positive_only_renders: ", strlen($bytes_pos) > 1024 ? "ok" : "bad", "\n";
?>
--EXPECT--
off_no_shelf: no
on_has_shelf: yes
positive_only_renders: ok
