--TEST--
VectorChart applies axis-label colors to both axes and preserves fallback and visibility
--EXTENSIONS--
fastchart
--FILE--
<?php
function vectorChart(): FastChart\VectorChart {
    return (new FastChart\VectorChart(400, 400))
        ->setVectors([
            ['x' => 0, 'y' => 0, 'dx' => 1, 'dy' => 1],
            ['x' => 3, 'y' => 3, 'dx' => 1, 'dy' => 1],
        ])
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        // Native SVG does not open the font file; avoid host font dependence.
        ->setAxisFont(__FILE__, 12);
}
function labels(string $svg): array {
    preg_match_all('/<text\b[^>]*fill="(#[0-9a-f]{6})"[^>]*text-anchor="([^"]+)"[^>]*>([^<]*)<\/text>/i', $svg, $m, PREG_SET_ORDER);
    return array_map(static fn(array $row): array => [strtolower($row[1]), $row[2], $row[3]], $m);
}
function check(string $name, FastChart\VectorChart $chart, string $color, int $x = 4, int $y = 4): void {
    $actual = labels($chart->renderSvg());
    $expected = [];
    for ($i = 0; $i < $y; $i++) $expected[] = [$color, 'end', (string)$i];
    for ($i = 0; $i < $x; $i++) $expected[] = [$color, 'middle', (string)$i];
    echo $name, ': ', $actual === $expected ? 'OK' : 'FAIL ' . json_encode($actual), "\n";
}

$theme = vectorChart()->renderSvg();
$chart = vectorChart()->setTextColor(0x135724);
$fallback = $chart->renderSvg();
check('text fallback', $chart, '#135724');
check('override', $chart->setAxisLabelColor(0xAB12CD), '#ab12cd');
check('black override', $chart->setAxisLabelColor(0), '#000000');
check('white override', $chart->setAxisLabelColor(0xFFFFFF), '#ffffff');
echo 'reset to text: ', $chart->setAxisLabelColor(-1)->renderSvg() === $fallback ? 'OK' : 'FAIL', "\n";
echo 'reset to theme: ', $chart->setTextColor(-1)->renderSvg() === $theme ? 'OK' : 'FAIL', "\n";

$dark = vectorChart()->setTheme(FastChart\Chart::THEME_DARK);
$darkFallback = $dark->renderSvg();
check('dark override', $dark->setAxisLabelColor(0xAB12CD), '#ab12cd');
echo 'reset to dark theme: ', $dark->setAxisLabelColor(-1)->renderSvg() === $darkFallback ? 'OK' : 'FAIL', "\n";

$chart = vectorChart()->setAxisLabelColor(0xAB12CD);
check('only X', $chart->setYAxisVisible(false), '#ab12cd', 4, 0);
check('only Y', $chart->setYAxisVisible(true)->setXAxisVisible(false), '#ab12cd', 0, 4);
check('both hidden', $chart->setYAxisVisible(false), '#ab12cd', 0, 0);
check('thumbnail', $chart->setXAxisVisible(true)->setYAxisVisible(true)->setThumbnailMode(true), '#ab12cd', 0, 0);

// The override changes text paint only, not tick positions, grids, or arrows.
$plain = vectorChart()->renderSvg();
$colored = vectorChart()->setAxisLabelColor(0xAB12CD)->renderSvg();
$withoutTextColor = static fn(string $svg): string => preg_replace('/(<text\b[^>]*fill=")#[0-9a-f]{6}/i', '${1}COLOR', $svg);
echo 'geometry unchanged: ', $withoutTextColor($plain) === $withoutTextColor($colored) ? 'OK' : 'FAIL', "\n";
?>
--EXPECT--
text fallback: OK
override: OK
black override: OK
white override: OK
reset to text: OK
reset to theme: OK
dark override: OK
reset to dark theme: OK
only X: OK
only Y: OK
both hidden: OK
thumbnail: OK
geometry unchanged: OK
