<?php

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
            if (imagecolorat($im, $x, $y) === 0x333333) $n++;
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
