--TEST--
LineChart and ScatterChart auto ranges include error-bar extents
--EXTENSIONS--
fastchart
--FILE--
<?php

function maximum_tick(FastChart\Chart $chart): float
{
    $svg = $chart
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->renderSvg();
    preg_match_all('/<text[^>]*>(-?\d+(?:\.\d+)?)<\/text>/', $svg, $m);
    return max(array_map('floatval', $m[1]));
}

$line = (new FastChart\LineChart(300, 200))
    ->setSeries([10, 20])
    ->setErrorBars([0, 100]);
$scatter = (new FastChart\ScatterChart(300, 200))
    ->setPoints([[0, 10], [1, 20]])
    ->setErrorBars([0, 100]);
$right = (new FastChart\LineChart(300, 200))
    ->setSecondaryYAxis(true)
    ->setSeries([
        ['data' => [10, 20], 'axis' => 'right'],
        ['data' => [1, 2], 'axis' => 'left'],
    ])
    ->setErrorBars([0, 100]);

echo 'line includes 120: ', maximum_tick($line) >= 120 ? "yes\n" : "no\n";
echo 'scatter includes 120: ', maximum_tick($scatter) >= 120 ? "yes\n" : "no\n";
echo 'right axis includes 120: ', maximum_tick($right) >= 120 ? "yes\n" : "no\n";

?>
--EXPECT--
line includes 120: yes
scatter includes 120: yes
right axis includes 120: yes
