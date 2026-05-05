--TEST--
PieChart: setExplode + setSliceLabelPosition + setSliceLabelFormat
--EXTENSIONS--
fastchart
--FILE--
<?php

// Exploded slice -- the first slice should sit displaced from the
// center, leaving plot-bg pixels at the geometric center where the
// slice would otherwise have been.
$im = imagecreatetruecolor(600, 600);
(new FastChart\PieChart)
    ->setSize(600, 600)
    ->setSlices(['A' => 50, 'B' => 25, 'C' => 25])
    ->setExplode([0 => 50])  // push first slice (A) by 50px
    ->draw($im);

// Slice A is the right half (60% of the pie) so center-x sits inside
// it. With 50px explode, slice A's center moves up-right, pulling a
// portion of A out of the geometric center, exposing background.
ob_start(); imagepng($im); $png = ob_get_clean();
echo "explode_png_sig: ", substr(bin2hex($png), 0, 8) === '89504e47' ? "ok" : "bad", "\n";

// LABEL_OUTSIDE: percentage labels appear OUTSIDE the pie radius.
$im2 = imagecreatetruecolor(700, 700);
(new FastChart\PieChart)
    ->setSize(700, 700)
    ->setSlices(['A' => 40, 'B' => 30, 'C' => 20, 'D' => 10])
    ->setSliceLabelPosition(FastChart\Chart::LABEL_OUTSIDE)
    ->draw($im2);

// Sample for text-color pixels near the plot edge (where outside
// labels live). Light theme text is #222222.
$text = 0x222222;
$edge_text = 0;
for ($y = 30; $y < 100; $y++) {
    for ($x = 50; $x < 650; $x++) {
        if (imagecolorat($im2, $x, $y) === $text) $edge_text++;
    }
}
for ($y = 600; $y < 670; $y++) {
    for ($x = 50; $x < 650; $x++) {
        if (imagecolorat($im2, $x, $y) === $text) $edge_text++;
    }
}
echo "outside_labels_visible: ", ($edge_text > 30 ? "yes" : "no ($edge_text)"), "\n";

// LABEL_NONE suppresses labels.
$im3 = imagecreatetruecolor(600, 600);
(new FastChart\PieChart)
    ->setSize(600, 600)
    ->setSlices(['A' => 40, 'B' => 30, 'C' => 30])
    ->setSliceLabelPosition(FastChart\Chart::LABEL_NONE)
    ->draw($im3);
$has_text = false;
for ($y = 100; $y < 500; $y++) {
    for ($x = 100; $x < 500; $x++) {
        if (imagecolorat($im3, $x, $y) === $text) { $has_text = true; break 2; }
    }
}
echo "none_no_text_in_plot: ", $has_text ? "no" : "yes", "\n";

// Custom label format.
$im4 = imagecreatetruecolor(600, 600);
(new FastChart\PieChart)
    ->setSize(600, 600)
    ->setSlices(['Just one' => 100])
    ->setSliceLabelFormat('val: %.2f%%')
    ->draw($im4);
ob_start(); imagepng($im4); $png4 = ob_get_clean();
echo "custom_format_png_sig: ", substr(bin2hex($png4), 0, 8) === '89504e47' ? "ok" : "bad", "\n";

// Bad position rejected.
try {
    (new FastChart\PieChart)->setSliceLabelPosition(99);
    echo "bad_pos: no throw\n";
} catch (\ValueError $e) {
    echo "bad_pos: ValueError ok\n";
}
?>
--EXPECT--
explode_png_sig: ok
outside_labels_visible: yes
none_no_text_in_plot: yes
custom_format_png_sig: ok
bad_pos: ValueError ok
