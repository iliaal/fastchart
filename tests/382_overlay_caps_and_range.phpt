--TEST--
Chart::addOverlaySeries(): input caps and overlay values participate in ranges
--EXTENSIONS--
fastchart
--INI--
asan.detect_leaks=0
--FILE--
<?php

function axis_has(string $svg, string $label): string {
    return str_contains($svg, '>' . $label . '<') ? 'yes' : 'no';
}

try {
    (new FastChart\LineChart(400, 250))
        ->setSeries([1, 2, 3])
        ->addOverlaySeries('line', array_fill(0, 2049, 1));
    echo "overlay point cap: accepted\n";
} catch (ValueError $e) {
    echo "overlay point cap: valueerror\n";
}

$cap = (new FastChart\LineChart(400, 250))->setSeries([1, 2, 3]);
for ($i = 0; $i < 16; $i++) {
    $cap->addOverlaySeries('line', [1, 2, 3]);
}
try {
    $cap->addOverlaySeries('line', [1, 2, 3]);
    echo "overlay count cap: accepted\n";
} catch (ValueError $e) {
    echo "overlay count cap: valueerror\n";
}

$line = (new FastChart\LineChart(500, 320))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSeries([1, 2, 3])
    ->addOverlaySeries('line', [60, 60, 60])
    ->renderSvg();
echo "line overlay range: ", axis_has($line, "60"), "\n";

$area = (new FastChart\AreaChart(500, 320))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSeries([1, 2, 3])
    ->addOverlaySeries('line', [60, 60, 60])
    ->renderSvg();
echo "area overlay range: ", axis_has($area, "60"), "\n";

$bar = (new FastChart\BarChart(500, 320))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSeries([1, 2, 3])
    ->addOverlaySeries('line', [60, 60, 60])
    ->renderSvg();
echo "bar overlay range: ", axis_has($bar, "60"), "\n";

?>
--EXPECT--
overlay point cap: valueerror
overlay count cap: valueerror
line overlay range: yes
area overlay range: yes
bar overlay range: yes
