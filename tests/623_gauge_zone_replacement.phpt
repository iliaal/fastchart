--TEST--
GaugeChart zone storage replacement preserves parsing, clones, and rendering
--EXTENSIONS--
fastchart
--FILE--
<?php
function gauge(int $style): FastChart\GaugeChart {
    return (new FastChart\GaugeChart(320, 240))
        ->setRange(0, 100)->setValue(98)->setStyle($style);
}
$full = [];
for ($i = 0; $i < 16; $i++) {
    $full[] = ['from' => $i * 6.25, 'to' => ($i + 1) * 6.25,
               'color' => 0x123400 + $i * 11];
}
$zone = ['from' => 100.0, 'to' => 0, 'color' => 0];
$mixed = [7 => null, 'missing' => ['from' => 0], 'zone' => &$zone,
          'string' => ['from' => '0', 'to' => '100'],
          'nan' => ['from' => NAN, 'to' => 100],
          'infinite' => ['from' => 0, 'to' => INF]];
$normalized = [['from' => 0, 'to' => 100, 'color' => 0]];
$emptyWidth = [['from' => 25, 'to' => 25]];

foreach ([FastChart\GaugeChart::STYLE_NEEDLE, FastChart\GaugeChart::STYLE_SOLID] as $style) {
    $chart = gauge($style);
    foreach ([$full, $mixed, $emptyWidth, [], $full, [null], $normalized] as $index => $zones) {
        $expected = $index === 1 ? $normalized : $zones;
        $reference = gauge($style)->setZones($expected)->renderSvg();
        $fluent = $chart->setZones($zones) === $chart;
        $copy = clone $chart;
        $actual = $chart->renderSvg();
        if (($index === 0 || $index === 4)
            && !str_contains($actual, sprintf('#%06X', $full[15]['color']))) {
            echo "style $style replacement $index: last zone missing\n";
        }
        $chart->setZones([]);
        if (!$fluent || $actual !== $reference || $copy->renderSvg() !== $reference
            || $copy->renderSvg() !== $actual) {
            echo "style $style replacement $index: BAD\n";
        }
        /* Reuse the populated object so subsequent iterations grow/shrink it. */
        $chart = $copy;
    }
    $before = $chart->renderSvg();
    try {
        $chart->setZones(array_fill(0, 17, null));
        echo "style $style cap did not throw\n";
    } catch (ValueError $e) {
        if ($chart->renderSvg() !== $before) echo "style $style cap changed state\n";
    }
    echo "style $style replacements checked\n";
}
?>
--EXPECT--
style 0 replacements checked
style 1 replacements checked
