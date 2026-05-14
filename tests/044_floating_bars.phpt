--TEST--
BarChart::setFloating draws bars between [min, max] (Gantt-style)
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

// Salary-range style: each entry is [low, high]. Slot 0 is
// intentionally interior to the global [dmin, dmax] = [10, 200]
// so its bar floats well above the plot baseline.
$bytes = (new FastChart\BarChart(600, 400))
    ->setFloating(true)
    ->setSeries([
        [60, 100],     // tested slot: floats inside [10, 200]
        [10, 200],     // widens the y range
        [70, 140],
        [90, 180],
    ])
    ->setCategoryLabels(['A', 'B', 'C', 'D'])
    ->renderPng();
echo "floating_renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

$im = imagecreatefromstring($bytes);

// The lowest band entry [40000, 80000] starts ABOVE zero, so the
// bar should not extend down to the X-axis baseline. Sample a
// vertical column over the first bar slot.
$series_color = (0x1f << 16) | (0x77 << 8) | 0xb4;
// Find the topmost and bottommost series-color pixel column over a
// reasonable x range for the first bar (~slot 0).
$top = 9999; $bot = -1;
for ($x = 60; $x < 180; $x++) {
    for ($y = 0; $y < 400; $y++) {
        if (imagecolorat($im, $x, $y) === $series_color) {
            if ($y < $top) $top = $y;
            if ($y > $bot) $bot = $y;
        }
    }
}
$has_bar = ($top < 9999 && $bot > 0);
echo "first_bar_present: ", $has_bar ? "yes" : "no", "\n";

// Slot 0's [60, 100] floats inside the global [10, 200] range, so
// the bar should be in the upper-middle region: top edge well below
// the plot top, bottom edge well above the plot bottom.
echo "first_bar_floats_top: ",    ($has_bar && $top > 50  ? "yes" : "no (top=$top)"), "\n";
echo "first_bar_floats_bottom: ", ($has_bar && $bot < 320 ? "yes" : "no (bot=$bot)"), "\n";

// Edge color works on floating bars too.
$bytes2 = (new FastChart\BarChart(600, 400))
    ->setFloating(true)
    ->setEdgeColor(0x000000)
    ->setSeries([[10, 20], [15, 25], [12, 22]])
    ->renderPng();
echo "floating_with_edge: ", strlen($bytes2) > 1024 ? "ok" : "bad", "\n";
?>
--EXPECT--
floating_renders: ok
first_bar_present: yes
first_bar_floats_top: yes
first_bar_floats_bottom: yes
floating_with_edge: ok
