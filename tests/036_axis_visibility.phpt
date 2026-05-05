--TEST--
setXAxisVisible / setYAxisVisible suppress axis line, ticks, and labels
--EXTENSIONS--
fastchart
--FILE--
<?php

// Reference render: both axes visible.
$ref = (new FastChart\LineChart(400, 300))
    ->setSeries([1, 5, 3, 8])
    ->renderPng();
$im_ref = imagecreatefromstring($ref);

// X axis hidden: the bottom row of the plot area should have far
// fewer axis-color (#333333) pixels.
$bytes = (new FastChart\LineChart(400, 300))
    ->setXAxisVisible(false)
    ->setSeries([1, 5, 3, 8])
    ->renderPng();
$im = imagecreatefromstring($bytes);

$count_axis = function ($im, $y, $w) {
    $n = 0;
    for ($x = 0; $x < $w; $x++) {
        if (imagecolorat($im, $x, $y) === 0x333333) $n++;
    }
    return $n;
};

// Sample around the lower portion of the plot for any horizontal
// axis-color stretch (>= 50 contiguous-ish px). Reference should
// have one; the hidden version shouldn't.
$has_axis_line = function ($im, $w, $h) {
    for ($y = 200; $y < $h; $y++) {
        $n = 0;
        for ($x = 30; $x < $w - 30; $x++) {
            if (imagecolorat($im, $x, $y) === 0x333333) $n++;
        }
        if ($n > 100) return true;
    }
    return false;
};

echo "ref_has_x_axis: ", $has_axis_line($im_ref, 400, 300) ? "yes" : "no", "\n";
echo "hidden_no_x_axis: ", $has_axis_line($im, 400, 300) ? "no" : "yes", "\n";

// Y axis hidden: same idea on the left edge.
$bytes2 = (new FastChart\LineChart(400, 300))
    ->setYAxisVisible(false)
    ->setSeries([1, 5, 3, 8])
    ->renderPng();
$im2 = imagecreatefromstring($bytes2);

$has_y_axis = function ($im, $w, $h) {
    for ($x = 0; $x < 100; $x++) {
        $n = 0;
        for ($y = 30; $y < $h - 30; $y++) {
            if (imagecolorat($im, $x, $y) === 0x333333) $n++;
        }
        if ($n > 100) return true;
    }
    return false;
};
echo "ref_has_y_axis: ", $has_y_axis($im_ref, 400, 300) ? "yes" : "no", "\n";
echo "hidden_no_y_axis: ", $has_y_axis($im2, 400, 300) ? "no" : "yes", "\n";
?>
--EXPECT--
ref_has_x_axis: yes
hidden_no_x_axis: yes
ref_has_y_axis: yes
hidden_no_y_axis: yes
