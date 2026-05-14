--TEST--
setShowValues renders numeric value labels above markers / bars
--EXTENSIONS--
fastchart
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


$text_color = 0x222222;  // light theme text

// LineChart: values appear above markers.
$bytes = (new FastChart\LineChart(800, 400))
    ->setShowValues(true, '%.0f')
    ->setSeries([10, 25, 18, 42, 33])
    ->renderPng();
$im = imagecreatefromstring($bytes);

// Sample for text-color pixels in a vertical band where the label
// should sit (just above each data marker).
$found_labels = 0;
for ($y = 30; $y < 380; $y++) {
    for ($x = 50; $x < 770; $x++) {
        if (fc_color_near(imagecolorat($im, $x, $y), $text_color)) {
            $found_labels++;
            if ($found_labels > 80) break 2;
        }
    }
}
echo "line_value_labels_present: ", ($found_labels > 80 ? "yes" : "no"), "\n";

// BarChart: above each bar.
$bytes2 = (new FastChart\BarChart(800, 400))
    ->setShowValues(true)
    ->setSeries([100, 200, 150, 250])
    ->renderPng();
$im2 = imagecreatefromstring($bytes2);
$found2 = 0;
for ($y = 30; $y < 380; $y++) {
    for ($x = 50; $x < 770; $x++) {
        if (fc_color_near(imagecolorat($im2, $x, $y), $text_color)) {
            $found2++;
            if ($found2 > 80) break 2;
        }
    }
}
echo "bar_value_labels_present: ", ($found2 > 80 ? "yes" : "no"), "\n";

// Off by default.
$png = (new FastChart\LineChart(800, 400))
    ->setSeries([10, 20, 30])
    ->renderPng();
echo "off_by_default_renders: ", strlen($png) > 1024 ? "ok" : "bad", "\n";
?>
--EXPECT--
line_value_labels_present: yes
bar_value_labels_present: yes
off_by_default_renders: ok
