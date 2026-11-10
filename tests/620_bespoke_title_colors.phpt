--TEST--
Bespoke chart titles honor color overrides, fallback, reset and clone without changing other SVG output
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die('skip no system font');
?>
--FILE--
<?php
require __DIR__ . '/_bespoke_title_charts.inc';
require __DIR__ . '/_font_candidates.inc';
$font = fc_pick_font();

function titleColor(string $svg): ?string {
    preg_match_all('/<text\b[^>]*>TITLE_SENTINEL<\/text>/', $svg, $titles);
    if (count($titles[0]) !== 1) return null;
    preg_match('/fill="(#[0-9a-f]{6})"/i', $titles[0][0], $color);
    return isset($color[1]) ? strtolower($color[1]) : null;
}
function normalizeTitleColor(string $svg): string {
    // Preserve the title's position/font and all non-title text, paint and geometry.
    return preg_replace_callback('/<text\b[^>]*>TITLE_SENTINEL<\/text>/',
        static fn(array $m): string => preg_replace('/fill="#[0-9a-f]{6}"/i', 'fill="COLOR"', $m[0]), $svg);
}
function check(bool $ok, string $label, array &$failures): void {
    if (!$ok) $failures[] = $label;
}

foreach (bespokeTitleCharts() as $name => $chart) {
    $failures = [];
    $chart->setTitle('TITLE_SENTINEL')->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        // Title layout measures the font even when SVG emits native text.
        ->setFontPath($font);
    $theme = $chart->renderSvg();
    check(titleColor($theme) === '#222222', 'light theme', $failures);
    $fallback = $chart->setTextColor(0x135724)->renderSvg();
    check(titleColor($fallback) === '#135724', 'text fallback', $failures);
    foreach ([0xAB12CD, 0, 0xFFFFFF] as $color) {
        $svg = $chart->setTitleColor($color)->renderSvg();
        check(titleColor($svg) === sprintf('#%06x', $color), sprintf('override %06x', $color), $failures);
        check(normalizeTitleColor($svg) === normalizeTitleColor($fallback), 'only title paint changes', $failures);
    }
    check($chart->setTitleColor(-1)->renderSvg() === $fallback, 'reset to text', $failures);
    check($chart->setTextColor(-1)->renderSvg() === $theme, 'reset to theme', $failures);
    $dark = $chart->setTheme(FastChart\Chart::THEME_DARK)->renderSvg();
    check(titleColor($dark) === '#eeeeee', 'dark theme', $failures);
    check(titleColor($chart->setTitleColor(0xAB12CD)->renderSvg()) === '#ab12cd', 'dark override', $failures);
    check($chart->setTitleColor(-1)->renderSvg() === $dark, 'reset to dark theme', $failures);

    $chart->setTitleColor(0xAB12CD)->setTextColor(0x246813);
    $custom = $chart->renderSvg();
    check(titleColor($custom) === '#ab12cd', 'title wins over changed text', $failures);
    check($chart->renderSvg() === $custom, 'repeated render', $failures);
    $copy = clone $chart;
    check($copy->renderSvg() === $custom, 'clone retains override', $failures);
    check(titleColor($copy->setTitleColor(-1)->renderSvg()) === '#246813', 'clone reset uses changed text', $failures);
    check($chart->renderSvg() === $custom, 'clone reset leaves original unchanged', $failures);

    $chart->setTitle('');
    $empty = $chart->renderSvg();
    check(!str_contains($empty, 'TITLE_SENTINEL'), 'empty title omitted', $failures);
    check($chart->setTitleColor(-1)->renderSvg() === $empty, 'empty title ignores override', $failures);
    echo $name, ': ', $failures ? 'FAIL ' . implode(', ', $failures) : 'OK', "\n";
}
?>
--EXPECT--
ArcDiagram: OK
BulletChart: OK
CalendarHeatmap: OK
ChordDiagram: OK
CirclePacking: OK
Dendrogram: OK
Funnel: OK
LinearMeter: OK
MarimekkoChart: OK
NetworkChart: OK
ParetoChart: OK
Partition: OK
Pictogram: OK
PopulationPyramid: OK
SankeyChart: OK
SerpentineTimeline: OK
SunburstChart: OK
Treemap: OK
VectorChart: OK
VennDiagram: OK
WordCloud: OK
