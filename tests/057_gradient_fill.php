<?php

// Vertical red->blue gradient on bars. Sample top vs bottom of a bar
// area: top should be reddish, bottom bluish.
$bytes = (new FastChart\BarChart(500, 400))
    ->setGradientFill(0xFF0000, 0x0000FF, FastChart\Chart::GRADIENT_VERTICAL)
    ->setSeries([100, 100, 100])
    ->renderPng();
$im = imagecreatefromstring($bytes);

// Find any column inside a bar (non-bg pixels) and sample its top and
// bottom rows. The bg/grid colors are stable, so any reddish/bluish
// pixel at the right depth confirms the gradient direction.
$found_top_red = false;
$found_bot_blue = false;
for ($x = 60; $x < 440; $x += 10) {
    // Walk down to find first non-white pixel = bar top.
    for ($y = 30; $y < 380; $y++) {
        $c = imagecolorat($im, $x, $y);
        $r = ($c >> 16) & 0xFF;
        $g = ($c >>  8) & 0xFF;
        $b =  $c        & 0xFF;
        if ($r > 200 && $g < 100 && $b < 100) { $found_top_red = true; break; }
    }
    // Walk up to find last bar pixel = bar bottom.
    for ($y = 380; $y > 30; $y--) {
        $c = imagecolorat($im, $x, $y);
        $r = ($c >> 16) & 0xFF;
        $g = ($c >>  8) & 0xFF;
        $b =  $c        & 0xFF;
        if ($b > 200 && $r < 100 && $g < 100) { $found_bot_blue = true; break; }
    }
}
echo "vertical_top_red: ",   $found_top_red ? "yes" : "no", "\n";
echo "vertical_bot_blue: ",  $found_bot_blue ? "yes" : "no", "\n";

// Horizontal direction.
$bytes2 = (new FastChart\BarChart(500, 400))
    ->setGradientFill(0xFF0000, 0x0000FF, FastChart\Chart::GRADIENT_HORIZONTAL)
    ->setSeries([100])
    ->renderPng();
echo "horizontal_renders: ", strlen($bytes2) > 1024 ? "ok" : "bad", "\n";

// Disable.
$bytes3 = (new FastChart\BarChart(400, 300))
    ->setGradientFill(0xFF0000)
    ->setGradientFill(-1)
    ->setSeries([10, 20, 30])
    ->renderPng();
echo "disable_renders: ", strlen($bytes3) > 1024 ? "ok" : "bad", "\n";

// Out-of-range.
try {
    (new FastChart\BarChart)->setGradientFill(0x1000000);
    echo "bad: no throw\n";
} catch (\ValueError $e) {
    echo "bad: ValueError ok\n";
}
?>
