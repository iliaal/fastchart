--TEST--
PieChart::setOtherThreshold aggregates small slices into Other
--EXTENSIONS--
fastchart
--FILE--
<?php

// 6 slices: Big (70), Med (20), then four tiny ones.
$slices = [
    'Big'   => 70,
    'Med'   => 20,
    'Tiny1' => 3,
    'Tiny2' => 3,
    'Tiny3' => 2,
    'Tiny4' => 2,
];

// Without threshold -- all 6 slices visible (6 distinct palette colors).
$bytes = (new FastChart\PieChart(600, 600))
    ->setSlices($slices)
    ->renderPng();
$im = imagecreatefromstring($bytes);

$palette = [0x1f77b4, 0xff7f0e, 0x2ca02c, 0xd62728, 0x9467bd, 0x8c564b];
$found_no_thr = 0;
for ($y = 80; $y < 540; $y += 4) {
    for ($x = 60; $x < 540; $x += 4) {
        $c = imagecolorat($im, $x, $y);
        foreach ($palette as $p) {
            if ($c === $p) { $found_no_thr++; break; }
        }
    }
}
echo "no_threshold_many_colors: ", ($found_no_thr > 200 ? "yes" : "no"), "\n";

// With threshold 5%: tiny slices (<5%) collapse into one "Other".
// Now only 3 distinct palette colors should dominate (Big, Med, Other).
$bytes2 = (new FastChart\PieChart(600, 600))
    ->setSlices($slices)
    ->setOtherThreshold(5.0)
    ->renderPng();
$im2 = imagecreatefromstring($bytes2);

// Count distinct palette colors that show up.
$present = 0;
foreach ($palette as $p) {
    for ($y = 80; $y < 540; $y += 4) {
        for ($x = 60; $x < 540; $x += 4) {
            if (imagecolorat($im2, $x, $y) === $p) { $present++; break 2; }
        }
    }
}
echo "threshold_collapses_to: ", $present, " distinct palette colors\n";

// Custom Other label.
$bytes = (new FastChart\PieChart(500, 500))
    ->setSlices(['A' => 50, 'B' => 30, 'X' => 5, 'Y' => 5, 'Z' => 5, 'W' => 5])
    ->setOtherThreshold(8.0, 'Smaller')
    ->renderPng();
echo "custom_other_label: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

// 0 disables.
$out = (new FastChart\PieChart(500, 500))
    ->setSlices(['A' => 1, 'B' => 1, 'C' => 1, 'D' => 1, 'E' => 1])
    ->setOtherThreshold(0.0)
    ->renderPng();
echo "zero_disables: ", strlen($out) > 1024 ? "ok" : "bad", "\n";

// Out-of-range rejected.
try {
    (new FastChart\PieChart)->setOtherThreshold(150.0);
    echo "bad_threshold: no throw\n";
} catch (\ValueError $e) {
    echo "bad_threshold: ValueError ok\n";
}
?>
--EXPECT--
no_threshold_many_colors: yes
threshold_collapses_to: 3 distinct palette colors
custom_other_label: ok
zero_disables: ok
bad_threshold: ValueError ok
