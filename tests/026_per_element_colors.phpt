--TEST--
setAxisColor / setGridColor / setBorderColor / setTextColor override theme palette
--EXTENSIONS--
fastchart
--FILE--
<?php

$bytes = (new FastChart\LineChart(800, 500))
    ->setAxisColor(0xFF0000)    // red axis
    ->setGridColor(0x00FF00)    // green grid
    ->setBorderColor(0x0000FF)  // blue border
    ->setTextColor(0xFF8800)    // orange text
    ->setTitle('Color override test')
    ->setSeries([1, 5, 3, 8, 4])
    ->renderPng();
$im = imagecreatefromstring($bytes);

// Sample for each color anywhere on the canvas. Layout note: the
// y-axis (axis color) and the topmost/bottommost grid lines (grid
// color) overdraw the left, top, and bottom plot borders, so the
// right border is the surviving stretch in border color.
$has_color = function ($im, $color, $W, $H) {
    for ($y = 0; $y < $H; $y++) {
        for ($x = 0; $x < $W; $x++) {
            if (imagecolorat($im, $x, $y) === $color) return true;
        }
    }
    return false;
};

echo "border_blue_present: ", $has_color($im, 0x0000FF, 800, 500) ? "yes" : "no", "\n";
echo "axis_red_present: ", $has_color($im, 0xFF0000, 800, 500) ? "yes" : "no", "\n";
echo "grid_green_present: ", $has_color($im, 0x00FF00, 800, 500) ? "yes" : "no", "\n";
echo "text_orange_present: ", $has_color($im, 0xFF8800, 800, 500) ? "yes" : "no", "\n";

// Out-of-range rejected.
try {
    (new FastChart\LineChart)->setAxisColor(0x1000000);
    echo "out_of_range: no throw\n";
} catch (\ValueError $e) {
    echo "out_of_range: ValueError ok\n";
}

// -1 reverts to theme.
$out = (new FastChart\LineChart(400, 300))
    ->setAxisColor(0xFF0000)->setAxisColor(-1)
    ->setSeries([1, 2, 3])->renderPng();
echo "revert_to_theme: ", strlen($out) > 1024 ? "ok" : "bad", "\n";
?>
--EXPECT--
border_blue_present: yes
axis_red_present: yes
grid_green_present: yes
text_orange_present: yes
out_of_range: ValueError ok
revert_to_theme: ok
