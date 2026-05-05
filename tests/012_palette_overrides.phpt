--TEST--
setBackgroundColor / setPlotBackgroundColor / setSeriesColors override the theme
--EXTENSIONS--
fastchart
--FILE--
<?php

// Background override only.
$im = imagecreatetruecolor(400, 200);
(new FastChart\LineChart)
    ->setSize(400, 200)
    ->setBackgroundColor(0xFF00FF)
    ->setSeries([1, 5, 3, 7])
    ->draw($im);

// Top-left corner pixel should now be magenta.
$bg = imagecolorat($im, 5, 5);
printf("bg=#%06x\n", $bg);

// Plot bg defaults to canvas bg when only canvas is overridden.
// Sample inside the plot area (centered).
$plot_bg = imagecolorat($im, 200, 100);
printf("plot_bg_follows=#%06x\n", $plot_bg);

// Plot bg override independent of canvas bg.
$im2 = imagecreatetruecolor(400, 200);
(new FastChart\LineChart)
    ->setSize(400, 200)
    ->setBackgroundColor(0xFF00FF)
    ->setPlotBackgroundColor(0x000080)
    ->setSeries([1, 5, 3, 7])
    ->draw($im2);

$canvas_bg2 = imagecolorat($im2, 5, 5);
$plot_bg2   = imagecolorat($im2, 200, 100);
printf("indep_canvas=#%06x\n", $canvas_bg2);
printf("indep_plot=#%06x\n", $plot_bg2);

// Series colors override.
$im3 = imagecreatetruecolor(400, 300);
(new FastChart\LineChart)
    ->setSize(400, 300)
    ->setSeriesColors([0xFF0000, 0x00FF00])
    ->setSeries([
        ['label' => 'A', 'data' => [1, 5, 3]],
        ['label' => 'B', 'data' => [2, 4, 6]],
    ])
    ->draw($im3);

$red   = 0xFF0000;
$green = 0x00FF00;
$default_blue = 0x1f77b4;
$found_red = $found_green = $found_default = false;
for ($y = 60; $y < 260; $y += 3) {
    for ($x = 60; $x < 380; $x += 3) {
        $c = imagecolorat($im3, $x, $y);
        if ($c === $red) $found_red = true;
        if ($c === $green) $found_green = true;
        if ($c === $default_blue) $found_default = true;
    }
}
echo "series_red: ", $found_red ? "yes" : "no", "\n";
echo "series_green: ", $found_green ? "yes" : "no", "\n";
echo "default_palette_used: ", $found_default ? "yes" : "no", "\n";

// Out-of-range rejects.
try {
    (new FastChart\LineChart)->setBackgroundColor(0x1000000);
    echo "bg_out_of_range: no throw\n";
} catch (\ValueError $e) {
    echo "bg_out_of_range: ValueError ok\n";
}
try {
    (new FastChart\LineChart)->setSeriesColors([0xFF, "string"]);
    echo "series_bad_type: no throw\n";
} catch (\TypeError $e) {
    echo "series_bad_type: TypeError ok\n";
}
?>
--EXPECT--
bg=#ff00ff
plot_bg_follows=#ff00ff
indep_canvas=#ff00ff
indep_plot=#000080
series_red: yes
series_green: yes
default_palette_used: no
bg_out_of_range: ValueError ok
series_bad_type: TypeError ok
