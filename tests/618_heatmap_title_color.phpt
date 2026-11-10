--TEST--
Heatmap applies title color independently of cell-label contrast and geometry
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
$chart = (new FastChart\Heatmap(400, 300))->setGrid([[0, 1]])
    ->setTitle('Heading')->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
if (!str_contains($chart->renderSvg(), '>Heading</text>')) die('skip no usable default font');
?>
--FILE--
<?php
function chart(): FastChart\Heatmap {
    return (new FastChart\Heatmap(400, 300))->setGrid([[0, 1]])
        ->setColorRamp(0xFFFFFF, 0x000000)->setShowValues(true)
        ->setTitle('Heading')->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
}
function check(bool $ok, string $name): void {
    echo $name, ': ', $ok ? 'OK' : 'FAIL', "\n";
}
function titleColor(string $svg): string {
    preg_match('/<text\b[^>]*fill="([^"]+)"[^>]*>Heading<\/text>/', $svg, $m);
    return strtolower($m[1] ?? '');
}
function withoutTitleColor(string $svg): string {
    return preg_replace('/(<text\b[^>]*fill=")[^"]+("[^>]*>Heading<\/text>)/', '$1TITLE$2', $svg);
}
foreach ([FastChart\Chart::THEME_LIGHT, FastChart\Chart::THEME_DARK] as $theme) {
    $chart = chart()->setTheme($theme)->setTextColor(0x123456);
    $original = $chart->renderSvg();
    check(titleColor($original) === '#123456', "$theme text fallback");
    preg_match_all('/<text\b[^>]*fill="([^"]+)"[^>]*>([01])<\/text>/', $original, $m, PREG_SET_ORDER);
    check(array_map(fn($row) => strtolower($row[1]), $m) === ['#000000', '#ffffff'], "$theme cell contrast");
    foreach ([0xAB12CD, 0x000000, 0xFFFFFF] as $color) {
        $svg = $chart->setTitleColor($color)->renderSvg();
        check(titleColor($svg) === sprintf('#%06x', $color), "$theme color $color");
        check(withoutTitleColor($svg) === withoutTitleColor($original), "$theme only title $color");
        check($chart->renderSvg() === $svg && (clone $chart)->renderSvg() === $svg, "$theme stable $color");
    }
    check($chart->setTitleColor(-1)->renderSvg() === $original, "$theme reset to text");
    $default = chart()->setTheme($theme)->renderSvg();
    check($chart->setTextColor(-1)->renderSvg() === $default, "$theme reset to theme");
    $chart->setTitleColor(0xAB12CD)->setTextColor(0x654321);
    check(titleColor($chart->renderSvg()) === '#ab12cd', "$theme override precedence");
    $empty = $chart->setTitle('')->renderSvg();
    check($chart->setTitleColor(0)->renderSvg() === $empty, "$theme no title");
}
?>
--EXPECT--
0 text fallback: OK
0 cell contrast: OK
0 color 11211469: OK
0 only title 11211469: OK
0 stable 11211469: OK
0 color 0: OK
0 only title 0: OK
0 stable 0: OK
0 color 16777215: OK
0 only title 16777215: OK
0 stable 16777215: OK
0 reset to text: OK
0 reset to theme: OK
0 override precedence: OK
0 no title: OK
1 text fallback: OK
1 cell contrast: OK
1 color 11211469: OK
1 only title 11211469: OK
1 stable 11211469: OK
1 color 0: OK
1 only title 0: OK
1 stable 0: OK
1 color 16777215: OK
1 only title 16777215: OK
1 stable 16777215: OK
1 reset to text: OK
1 reset to theme: OK
1 override precedence: OK
1 no title: OK
