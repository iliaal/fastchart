--TEST--
ViolinPlot strides group labels without changing silhouettes, medians, or retained labels
--EXTENSIONS--
fastchart
--FILE--
<?php
function chart(bool $missing = false): FastChart\ViolinPlot {
    $groups = [];
    for ($i = 0; $i < 6; $i++) {
        $group = ['values' => [1, 2, 3 + $i, 5, 8]];
        if (!$missing || $i !== 2) $group['label'] = "Group$i";
        $groups[] = $group;
    }
    return (new FastChart\ViolinPlot(900, 400))->setGroups($groups)
        ->setFontPath(__FILE__)->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
}
function labels(string $svg): array {
    preg_match_all('/<text\b[^>]*>Group(\d+)<\/text>/', $svg, $matches);
    return array_map('intval', $matches[1]);
}
$chart = chart();
$default = $chart->renderSvg();
echo 'default: ', labels($default) === [0, 1, 2, 3, 4, 5] ? 'OK' : 'FAIL', "\n";
foreach ([1 => [0, 1, 2, 3, 4, 5], 2 => [0, 2, 4], 3 => [0, 3], 1000 => [0]] as $stride => $expected) {
    $svg = $chart->setXLabelStride($stride)->renderSvg();
    echo "stride $stride: ", labels($svg) === $expected ? 'OK' : 'FAIL', "\n";
    $filtered = preg_replace_callback('/<text\b[^>]*>Group(\d+)<\/text>\n?/',
        static fn(array $m): string => (int)$m[1] % $stride === 0 ? $m[0] : '', $default);
    echo "unchanged $stride: ", $svg === $filtered ? 'OK' : 'FAIL', "\n";
}
echo 'reset: ', $chart->setXLabelStride(1)->renderSvg() === $default ? 'OK' : 'FAIL', "\n";
echo 'missing label: ', labels(chart(true)->setXLabelStride(2)->renderSvg()) === [0, 4] ? 'OK' : 'FAIL', "\n";
// setGroups() compacts groups with no finite values before rendering.
$chart->setGroups([
    ['label' => 'Group0', 'values' => [1, 2, 3]],
    ['label' => 'Dropped', 'values' => [NAN, INF]],
    ['label' => 'Group1', 'values' => [2, 3, 4]],
    ['label' => 'Group2', 'values' => [3, 4, 5]],
]);
echo 'compacted groups: ', labels($chart->setXLabelStride(2)->renderSvg()) === [0, 2] ? 'OK' : 'FAIL', "\n";
?>
--EXPECT--
default: OK
stride 1: OK
unchanged 1: OK
stride 2: OK
unchanged 2: OK
stride 3: OK
unchanged 3: OK
stride 1000: OK
unchanged 1000: OK
reset: OK
missing label: OK
compacted groups: OK
