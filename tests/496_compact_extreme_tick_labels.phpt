--TEST--
Default numeric tick labels use compact exponent form at extreme magnitudes
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
echo labels((new FastChart\LineChart(400,300))->setSeries([1.7e308,1e307])
    ->setYAxisRange(-1.7e308,1.7e308)->setXAxisVisible(false)), "\n";
echo labels((new FastChart\LineChart(600,400))->setSeries([['data'=>[1e100]],['data'=>[1e100],'axis'=>'right']])
    ->setSecondaryYAxis(true)->setXAxisVisible(false)), "\n";
echo labels((new FastChart\BarChart(600,400))->setSeries([2e15])->setYAxisRange(0,2e15)
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)->setYAxisVisible(false)), "\n";
echo labels((new FastChart\ScatterChart(600,400))->setPoints([[0,0],[1e-8,1]])
    ->setYAxisVisible(false)), "\n";
echo labels((new FastChart\LineChart(400,300))->setSeries([1e15+3])
    ->setYAxisRange(1e15,1e15+10)->setXAxisVisible(false)), "\n";
echo labels((new FastChart\LineChart(400,300))->setSeries([50,-25])->setXAxisVisible(false)), "\n";
echo labels((new FastChart\LineChart(400,300))->setSeries([5e14])->setYAxisRange(0,5e14)->setXAxisVisible(false)), "\n";
$custom = explode(' | ', labels((new FastChart\LineChart(400,300))->setSeries([1.7e308])
    ->setYAxisRange(-1.7e308,1.7e308)->setYAxisLabelFormat('%.0f')->setXAxisVisible(false)));
echo "custom format complete: ", min(array_map('strlen', $custom)) >= 308 ? 'yes' : 'no', "\n";
?>
--EXPECT--
-1.7e+308 | -1.02e+308 | -3.4e+307 | 3.4e+307 | 1.02e+308 | 1.7e+308
9e+99 | 9.5e+99 | 1e+100 | 1.05e+100 | 1.1e+100 | 9e+99 | 9.5e+99 | 1e+100 | 1.05e+100 | 1.1e+100
0 | 5e+14 | 1e+15 | 1.5e+15 | 2e+15
0 | 2e-9 | 4e-9 | 6e-9 | 8e-9 | 1e-8
1e+15 | 1.000000000000002e+15 | 1.000000000000004e+15 | 1.000000000000006e+15 | 1.000000000000008e+15 | 1.00000000000001e+15
-40 | -20 | 0 | 20 | 40 | 60
0 | 100000000000000 | 200000000000000 | 300000000000000 | 400000000000000 | 500000000000000
custom format complete: yes
