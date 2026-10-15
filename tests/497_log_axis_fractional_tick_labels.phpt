--TEST--
Log-scale default tick labels show fractional decades instead of 0
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
$c = (new FastChart\LineChart(600,400))->setSeries([1,2])->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
if (!str_contains($c->renderSvg(), '<text ')) echo 'skip no usable default font';
?>
--FILE--
<?php
use FastChart\Chart;
function labels(Chart $c): string {
    preg_match_all('/<text\b[^>]*>([^<]*)<\/text>/', $c->setSvgTextMode(Chart::SVG_TEXT_NATIVE)->renderSvg(), $m);
    return implode(' | ', $m[1]);
}
$log = fn(array $d) => (new FastChart\LineChart(400,300))->setSeries($d)
    ->setYAxisScale(Chart::SCALE_LOG)->setXAxisVisible(false);
echo labels($log([0.001, 1000])), "\n";
echo labels($log([0.5, 50])), "\n";
echo labels($log([1e-6, 1])), "\n";
echo labels($log([1e-9, 10])), "\n";
echo labels($log([1, 1e6])), "\n";
echo labels((new FastChart\AreaChart(400,300))->setSeries([['data'=>[1,2]],['data'=>[0.01,10],'axis'=>'right']])
    ->setStacked(false)->setYAxisScale(Chart::SCALE_LOG)->setSecondaryYAxis(true)->setXAxisVisible(false)), "\n";
echo labels($log([0.001, 1000])->setYAxisLabelFormat('%.0f')), "\n";
?>
--EXPECT--
0.001 | 0.01 | 0.1 | 1 | 10 | 100 | 1000
0.1 | 1 | 10 | 100
0.000001 | 0.00001 | 0.0001 | 0.001 | 0.01 | 0.1 | 1
1e-9 | 1e-8 | 1e-7 | 1e-6 | 1e-5 | 1e-4 | 1e-3 | 1e-2 | 1e-1 | 1e+0 | 1e+1
1 | 10 | 100 | 1000 | 10000 | 100000 | 1000000
1 | 10 | 0.01 | 0.1 | 1 | 10
0 | 0 | 0 | 1 | 10 | 100 | 1000
