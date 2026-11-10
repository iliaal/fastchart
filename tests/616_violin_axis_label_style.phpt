--TEST--
ViolinPlot group labels use axis font sizes and colors independently of data-label roles
--EXTENSIONS--
fastchart
--FILE--
<?php
function chart(): FastChart\ViolinPlot {
    return (new FastChart\ViolinPlot(600, 400))->setGroups([
        ['label' => 'Alpha', 'values' => [1, 2, 3, 5]],
        ['label' => 'Beta', 'values' => [2, 3, 4, 6]],
    ])->setFontPath(__FILE__)->setFontSize(12)->setTextColor(0x123456)
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
}
function check(string $name, FastChart\ViolinPlot $chart, float $size, string $color): void {
    $svg = $chart->renderSvg();
    preg_match_all('/<text\b[^>]*font-size="([^"]+)"[^>]*fill="([^"]+)"[^>]*>(Alpha|Beta)<\/text>/', $svg, $rows, PREG_SET_ORDER);
    $ok = count($rows) === 2 && array_column($rows, 3) === ['Alpha', 'Beta'];
    foreach ($rows as $row) $ok = $ok && abs((float)$row[1] - $size * 4 / 3) < 0.051 && strtolower($row[2]) === $color;
    echo "$name: ", $ok && $chart->renderSvg() === $svg ? 'OK' : 'FAIL', "\n";
}
check('global fallback', chart(), 12, '#123456');
check('axis size', chart()->setAxisFont(null, 21), 21, '#123456');
check('label role ignored', chart()->setLabelFont(null, 30), 12, '#123456');
$chart = chart()->setAxisFont(null, 21)->setLabelFont(null, 30)->setAxisLabelColor(0xAB12CD);
check('both roles and color', $chart, 21, '#ab12cd');
$withoutLabels = static fn(string $svg): string => preg_replace('/<text\b[^>]*>(Alpha|Beta)<\/text>\n?/', '', $svg);
echo 'non-label geometry unchanged: ', $withoutLabels($chart->renderSvg()) === $withoutLabels(chart()->renderSvg()) ? 'OK' : 'FAIL', "\n";
check('black override', $chart->setAxisLabelColor(0), 21, '#000000');
check('reset color', $chart->setAxisLabelColor(-1), 21, '#123456');
check('updated size', $chart->setAxisFont(null, 18)->setLabelFont(null, 9), 18, '#123456');
check('thumbnail scaling', $chart->setThumbnailMode(true), 10.8, '#123456');
// Clearing an override must also restore theme-derived text colors.
$chart = chart()->setTextColor(-1);
$default = $chart->renderSvg();
echo 'theme fallback reset: ', $chart->setAxisLabelColor(0xAB12CD)->setAxisLabelColor(-1)->renderSvg() === $default ? 'OK' : 'FAIL', "\n";
?>
--EXPECT--
global fallback: OK
axis size: OK
label role ignored: OK
both roles and color: OK
non-label geometry unchanged: OK
black override: OK
reset color: OK
updated size: OK
thumbnail scaling: OK
theme fallback reset: OK
