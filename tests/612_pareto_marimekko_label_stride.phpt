--TEST--
ParetoChart and MarimekkoChart apply X-label stride without hiding values or segments
--EXTENSIONS--
fastchart
--FILE--
<?php
function chart(string $kind, bool $missing = false): FastChart\Chart {
    $items = [];
    for ($i = 0; $i < 6; $i++) {
        $item = $kind === 'pareto'
            ? ['value' => 60 - $i * 5]
            : ['segments' => [['label' => "Segment$i", 'value' => 60 - $i * 5]]];
        // A missing category must not shift the indices of later labels.
        if (!$missing || $i !== 2) $item['label'] = "Category$i";
        $items[] = $item;
    }
    $chart = $kind === 'pareto'
        ? (new FastChart\ParetoChart(900, 400))->setBars($items)->setShowValues(true)
        : (new FastChart\MarimekkoChart(900, 400))->setColumns($items);
    return $chart->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        // Native SVG does not open this file as a font.
        ->setFontPath(__FILE__)->setAxisLabelColor(0xAB12CD);
}
function categories(string $svg): array {
    preg_match_all('/<text\b[^>]*>Category(\d+)<\/text>/', $svg, $matches);
    return array_map('intval', $matches[1]);
}
function applyStride(string $svg, int $stride): string {
    return preg_replace_callback('/<text\b[^>]*>Category(\d+)<\/text>\n?/',
        static fn(array $m): string => (int)$m[1] % $stride === 0 ? $m[0] : '', $svg);
}
foreach (['pareto', 'marimekko'] as $kind) {
    $chart = chart($kind);
    $default = $chart->renderSvg();
    echo "$kind default: ", categories($default) === [0, 1, 2, 3, 4, 5] ? 'OK' : 'FAIL', "\n";
    foreach ([1 => [0, 1, 2, 3, 4, 5], 2 => [0, 2, 4], 3 => [0, 3], 1000 => [0]] as $stride => $expected) {
        $svg = $chart->setXLabelStride($stride)->renderSvg();
        echo "$kind stride $stride: ", categories($svg) === $expected ? 'OK' : 'FAIL', "\n";
        // This includes both Pareto Y axes, bar values, Mekko segment text,
        // every shape, and the coordinates and colors of all retained text.
        echo "$kind unchanged $stride: ", $svg === applyStride($default, $stride) ? 'OK' : 'FAIL', "\n";
    }
    echo "$kind reset: ", $chart->setXLabelStride(1)->renderSvg() === $default ? 'OK' : 'FAIL', "\n";
    echo "$kind missing category: ", categories(chart($kind, true)->setXLabelStride(2)->renderSvg()) === [0, 4] ? 'OK' : 'FAIL', "\n";
}
?>
--EXPECT--
pareto default: OK
pareto stride 1: OK
pareto unchanged 1: OK
pareto stride 2: OK
pareto unchanged 2: OK
pareto stride 3: OK
pareto unchanged 3: OK
pareto stride 1000: OK
pareto unchanged 1000: OK
pareto reset: OK
pareto missing category: OK
marimekko default: OK
marimekko stride 1: OK
marimekko unchanged 1: OK
marimekko stride 2: OK
marimekko unchanged 2: OK
marimekko stride 3: OK
marimekko unchanged 3: OK
marimekko stride 1000: OK
marimekko unchanged 1000: OK
marimekko reset: OK
marimekko missing category: OK
