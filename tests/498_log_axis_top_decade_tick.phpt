--TEST--
Log axes wider than the tick budget stride decades and keep both end decades
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
    ->setYAxisScale(Chart::SCALE_LOG)->setXAxisVisible(false)->setYAxisLabelFormat('%.0e');
echo labels($log([1, 1e15])), "\n";
echo labels($log([1, 1e16])), "\n";
echo labels($log([3, 2e16])), "\n";
echo labels($log([1e-300, 1e300])), "\n";
echo labels($log([1e-320, 1e308])), "\n";
echo labels($log([1e-322, 1e308])), "\n";
echo labels((new FastChart\AreaChart(400,300))->setSeries([['data'=>[1,10]],['data'=>[1,1e20],'axis'=>'right']])
    ->setStacked(false)->setYAxisScale(Chart::SCALE_LOG)->setSecondaryYAxis(true)
    ->setXAxisVisible(false)->setYAxisLabelFormat('%.0e')), "\n";
echo labels((new FastChart\LineChart(400,300))->setSeries([2, 5e5])
    ->setYAxisScale(Chart::SCALE_LOG)->setXAxisVisible(false)), "\n";
?>
--EXPECT--
1e+0 | 1e+1 | 1e+2 | 1e+3 | 1e+4 | 1e+5 | 1e+6 | 1e+7 | 1e+8 | 1e+9 | 1e+10 | 1e+11 | 1e+12 | 1e+13 | 1e+14 | 1e+15
1e+0 | 1e+2 | 1e+4 | 1e+6 | 1e+8 | 1e+10 | 1e+12 | 1e+14 | 1e+16
1e+0 | 1e+2 | 1e+4 | 1e+6 | 1e+8 | 1e+10 | 1e+12 | 1e+14 | 1e+16 | 1e+18
1e-300 | 1e-260 | 1e-220 | 1e-180 | 1e-140 | 1e-100 | 1e-60 | 1e-20 | 1e+20 | 1e+60 | 1e+100 | 1e+140 | 1e+180 | 1e+220 | 1e+260 | 1e+300
1e-322 | 1e-280 | 1e-238 | 1e-196 | 1e-154 | 1e-112 | 1e-70 | 1e-28 | 1e+14 | 1e+56 | 1e+98 | 1e+140 | 1e+182 | 1e+224 | 1e+266 | 1e+308
1e-323 | 1e-280 | 1e-237 | 1e-194 | 1e-151 | 1e-108 | 1e-65 | 1e-22 | 1e+21 | 1e+64 | 1e+107 | 1e+150 | 1e+193 | 1e+236 | 1e+279 | 1e+308
1e+0 | 1e+1 | 1e+0 | 1e+2 | 1e+4 | 1e+6 | 1e+8 | 1e+10 | 1e+12 | 1e+14 | 1e+16 | 1e+18 | 1e+20
1 | 10 | 100 | 1000 | 10000 | 100000 | 1000000
