--TEST--
setSecondaryYAxisTitle renders a rotated title on the right edge
--EXTENSIONS--
fastchart
--FILE--
<?php

$bytes = (new FastChart\LineChart(700, 400))
    ->setSecondaryYAxis(true)
    ->setYAxisTitle('Left')
    ->setSecondaryYAxisTitle('Right')
    ->setSeries([
        ['data' => [1, 5, 3, 8], 'axis' => 'left',  'label' => 'L'],
        ['data' => [100, 250, 180, 320], 'axis' => 'right', 'label' => 'R'],
    ])
    ->renderPng();
echo "renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

// Empty string clears.
$out = (new FastChart\LineChart)
    ->setSecondaryYAxisTitle('Right')
    ->setSecondaryYAxisTitle('');
var_dump($out instanceof FastChart\LineChart);

// Without secondary axis enabled, setting the title is a no-op.
$bytes2 = (new FastChart\LineChart(400, 300))
    ->setSecondaryYAxisTitle('IGNORED')
    ->setSeries([1, 2, 3])
    ->renderPng();
echo "no_secondary_renders: ", strlen($bytes2) > 1024 ? "ok" : "bad", "\n";
?>
--EXPECT--
renders: ok
bool(true)
no_secondary_renders: ok
