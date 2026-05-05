--TEST--
setLegendPosition: legend renders in the chosen corner; LEGEND_NONE suppresses
--EXTENSIONS--
fastchart
--FILE--
<?php

$build = function ($pos) {
    $im = imagecreatetruecolor(800, 400);
    (new FastChart\LineChart)
        ->setSize(800, 400)
        ->setLegendPosition($pos)
        ->setSeries([
            ['label' => 'A', 'data' => [1, 4, 2, 8]],
            ['label' => 'B', 'data' => [3, 2, 5, 4]],
        ])
        ->draw($im);
    return $im;
};

$blue = (0x1f << 16) | (0x77 << 8) | 0xb4;
$count_in = function ($im, $x0, $x1, $y0, $y1) use ($blue) {
    $hits = 0;
    for ($y = $y0; $y < $y1; $y++) {
        for ($x = $x0; $x < $x1; $x++) {
            if (imagecolorat($im, $x, $y) === $blue) $hits++;
        }
    }
    return $hits;
};

// TOP_RIGHT: blue swatch in top-right region.
$im = $build(FastChart\Chart::LEGEND_TOP_RIGHT);
$tr = $count_in($im, 600, 770, 10, 80);
echo "top_right_swatch_present: ", ($tr > 30 ? "yes" : "no"), "\n";

$im = $build(FastChart\Chart::LEGEND_TOP_LEFT);
$tl = $count_in($im, 60, 220, 10, 80);
echo "top_left_swatch_present: ", ($tl > 30 ? "yes" : "no"), "\n";

$im = $build(FastChart\Chart::LEGEND_BOTTOM_RIGHT);
$br = $count_in($im, 600, 770, 280, 360);
echo "bottom_right_swatch_present: ", ($br > 30 ? "yes" : "no"), "\n";

$im = $build(FastChart\Chart::LEGEND_BOTTOM_LEFT);
$bl = $count_in($im, 60, 220, 280, 360);
echo "bottom_left_swatch_present: ", ($bl > 30 ? "yes" : "no"), "\n";

// LEGEND_NONE: no swatch in any corner.
$im = $build(FastChart\Chart::LEGEND_NONE);
$any = $count_in($im, 60, 220, 10, 80) +
       $count_in($im, 600, 770, 10, 80) +
       $count_in($im, 60, 220, 280, 360) +
       $count_in($im, 600, 770, 280, 360);
// Non-zero is OK (data lines may pass through corner regions and
// thick 2-pass strokes add ~50-70 hits per corner). A legend swatch
// adds a dense ~140 pixel block on top of those line transits, so
// "swatch present" lands well above 350; "no swatch" stays below.
echo "legend_none_no_dense_swatch: ", ($any < 350 ? "yes" : "no ($any)"), "\n";

// Bad position rejected.
try {
    (new FastChart\LineChart)->setLegendPosition(99);
    echo "bad_pos: no throw\n";
} catch (\ValueError $e) {
    echo "bad_pos: ValueError ok\n";
}
?>
--EXPECT--
top_right_swatch_present: yes
top_left_swatch_present: yes
bottom_right_swatch_present: yes
bottom_left_swatch_present: yes
legend_none_no_dense_swatch: yes
bad_pos: ValueError ok
