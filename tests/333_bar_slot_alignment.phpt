--TEST--
BarChart: bar centers align with categorical label centers at high N
--EXTENSIONS--
fastchart
--INI--
asan.detect_leaks=0
--FILE--
<?php

/* Vertical bars positioned slots as plot.x0 + i * (int)(W/n), so the
 * truncated slot width accumulated tens of pixels of drift by the last
 * category — the last bar sat far left of its label/gridline. Slot
 * boundaries now come from double-precision division matching
 * fastchart_x_categorical_center(), so bar i centers on category i. */

$n = 100;
$data = $cats = [];
for ($i = 0; $i < $n; $i++) { $data[] = ($i % 20) + 5; $cats[] = "c$i"; }

$svg = (new FastChart\BarChart(800, 400))
    ->setSeries([['label' => 'S', 'data' => $data]])
    ->setCategoryLabels($cats)
    ->renderSvg();

/* Recover the plot rect from the horizontal gridlines: they span the
 * full plot width, so plot.x0 = min x1, plot.x1 = max x2. */
preg_match_all('/<line x1="(\d+)" y1="(\d+)" x2="(\d+)" y2="\2"[^>]*>/', $svg, $lm, PREG_SET_ORDER);
$plot_x0 = PHP_INT_MAX; $plot_x1 = 0;
foreach ($lm as $l) {
    $x1 = (int)$l[1]; $x2 = (int)$l[3];
    if ($x2 - $x1 > 400) {                 /* full-width plot line, not a tick stub */
        if ($x1 < $plot_x0) $plot_x0 = $x1;
        if ($x2 > $plot_x1) $plot_x1 = $x2;
    }
}

/* fastchart_x_categorical_center(plot, n-1, n). */
$step = ($plot_x1 - $plot_x0) / $n;
$expected_center = $plot_x0 + (int)($step * ($n - 1 + 0.5));

/* Rightmost bar fill rect = the last category's bar. */
preg_match_all('/<rect x="(\d+)" y="\d+" width="(\d+)" height="\d+" fill="(#[0-9A-Fa-f]{6})"\/>/', $svg, $rm, PREG_SET_ORDER);
$by_color = [];
foreach ($rm as $r) { $by_color[$r[3]][] = [(int)$r[1], (int)$r[2]]; }
/* The bar color is the one appearing exactly $n times. */
$bars = [];
foreach ($by_color as $rects) { if (count($rects) === $n) { $bars = $rects; break; } }
$max_x = -1; $last = null;
foreach ($bars as $b) { if ($b[0] > $max_x) { $max_x = $b[0]; $last = $b; } }
$last_center = $last[0] + $last[1] / 2;

$drift = abs($last_center - $expected_center);
echo 'plot recovered: ', ($plot_x0 < $plot_x1 ? 'yes' : 'no'), "\n";
echo 'bars found: ', (count($bars) === $n ? 'yes' : 'no (' . count($bars) . ')'), "\n";
echo 'last bar aligned: ', ($drift <= 2 ? 'yes' : "no (drift=$drift, center=$last_center, expected=$expected_center)"), "\n";

echo "done\n";
?>
--EXPECT--
plot recovered: yes
bars found: yes
last bar aligned: yes
done
