--TEST--
RadarChart: spider plot with multi-series filled polygons
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

// Single-series flat list.
$bytes = (new FastChart\RadarChart(500, 500))
    ->setCategoryLabels(['Speed', 'Power', 'Range', 'Eff', 'Cost', 'Style'])
    ->setSeries([80, 70, 60, 90, 40, 75])
    ->renderPng();
echo "single_renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

// Multi-series with explicit colors and labels.
$bytes2 = (new FastChart\RadarChart(500, 500))
    ->setCategoryLabels(['A', 'B', 'C', 'D'])
    ->setSeries([
        ['data' => [80, 70, 60, 90], 'label' => 'Alpha', 'color' => 0xFF0000],
        ['data' => [60, 80, 70, 50], 'label' => 'Beta',  'color' => 0x00CC00],
    ])
    ->renderPng();
$im = imagecreatefromstring($bytes2);

// Both colors should be present somewhere.
$has = function ($im, $c, $w, $h) {
    for ($y = 0; $y < $h; $y++)
        for ($x = 0; $x < $w; $x++)
            if (imagecolorat($im, $x, $y) === $c) return true;
    return false;
};
echo "alpha_red: ", $has($im, 0xFF0000, 500, 500) ? "yes" : "no", "\n";
echo "beta_green: ", $has($im, 0x00CC00, 500, 500) ? "yes" : "no", "\n";

// Filled vs outlined toggle.
$bytes3 = (new FastChart\RadarChart(500, 500))
    ->setSeries([10, 20, 30, 25, 18])
    ->setFilled(false)
    ->renderPng();
echo "outline_only: ", strlen($bytes3) > 1024 ? "ok" : "bad", "\n";

// Forced max value.
$bytes4 = (new FastChart\RadarChart(500, 500))
    ->setSeries([10, 20, 30])
    ->setMaxValue(100.0)
    ->renderPng();
echo "max_value: ", strlen($bytes4) > 1024 ? "ok" : "bad", "\n";

// Bad: too few axes.
try {
    (new FastChart\RadarChart(400, 400))
        ->setSeries([1, 2])
        ->renderPng();
    echo "few_axes: no throw\n";
} catch (\Error $e) {
    echo "few_axes: error ok\n";
}
?>
--EXPECT--
single_renders: ok
alpha_red: yes
beta_green: yes
outline_only: ok
max_value: ok
few_axes: error ok
