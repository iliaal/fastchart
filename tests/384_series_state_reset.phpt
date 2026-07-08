--TEST--
Series replacement clears parsed dependent state
--EXTENSIONS--
fastchart
--INI--
asan.detect_leaks=0
--FILE--
<?php

$line = (new FastChart\LineChart(420, 260))
    ->setSeries([10, 20, 15])
    ->setErrorBars([4, 4, 4]);
$line->setSeries([10, 20, 15]);
$fresh_line = (new FastChart\LineChart(420, 260))->setSeries([10, 20, 15]);
echo "line error bars cleared: ", ($line->renderSvg() === $fresh_line->renderSvg() ? 'yes' : 'no'), "\n";

$scatter = (new FastChart\ScatterChart(420, 260))
    ->setPoints([[1, 10], [2, 20], [3, 15]])
    ->setErrorBars([4, 4, 4]);
$scatter->setPoints([[1, 10], [2, 20], [3, 15]]);
$fresh_scatter = (new FastChart\ScatterChart(420, 260))
    ->setPoints([[1, 10], [2, 20], [3, 15]]);
echo "scatter error bars cleared: ", ($scatter->renderSvg() === $fresh_scatter->renderSvg() ? 'yes' : 'no'), "\n";

$bar = (new FastChart\BarChart(420, 260))
    ->setFloating(true)
    ->setSeries([[[1, 3], [2, 5]]]);
try {
    $bar->setFloating(false);
    echo "floating disable after data: accepted\n";
} catch (ValueError $e) {
    echo "floating disable after data: valueerror\n";
}

?>
--EXPECT--
line error bars cleared: yes
scatter error bars cleared: yes
floating disable after data: valueerror
