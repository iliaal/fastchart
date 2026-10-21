--TEST--
RadarChart: large finite values retain proportional geometry
--EXTENSIONS--
fastchart
--FILE--
<?php
function renderScaled(float $scale, bool $filled, bool $forced): string {
    $chart = (new FastChart\RadarChart(400, 400))
        ->setSeries([
            ['data' => [$scale, $scale / 2, $scale / 4, 0]],
            ['data' => [$scale / 4, 0, $scale / 2, $scale]],
        ])
        ->setFilled($filled);
    if ($forced) $chart->setMaxValue($scale * 2);
    return $chart->renderSvg();
}
foreach ([false, true] as $filled) {
    foreach ([false, true] as $forced) {
        $expected = renderScaled(1.0, $filled, $forced);
        foreach ([1e306, 1e307, 8e307] as $scale) {
            if (renderScaled($scale, $filled, $forced) !== $expected) {
                echo "geometry differs\n";
            }
        }
    }
}
/* An intentionally tiny forced maximum still saturates safely. */
$tiny = (new FastChart\RadarChart(400, 400))
    ->setSeries([1e308, 5e307, 2.5e307])
    ->setMaxValue(1e-300)->renderSvg();
$clamped = (new FastChart\RadarChart(400, 400))
    ->setSeries([32, 16, 8])->setMaxValue(1)->renderSvg();
var_dump($tiny === $clamped);
echo "OK\n";
?>
--EXPECT--
bool(true)
OK
