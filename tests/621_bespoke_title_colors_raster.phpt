--TEST--
Bespoke chart title overrides reach SVG glyph paths and PNG without changing non-title pixels
--EXTENSIONS--
fastchart
gd
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

foreach (bespokeTitleCharts() as $name => $chart) {
    $failures = [];
    $chart->setTitle('TITLE_SENTINEL')->setTitleFont($font, 18)->setFontPath($font)
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_PATHS)->setTextColor(0x135724);
    $plainSvg = $chart->renderSvg();
    $plainPng = $chart->renderPng();
    $chart->setTitleColor(0xAB12CD);
    $coloredSvg = $chart->renderSvg();
    $coloredPng = $chart->renderPng();
    if (!preg_match('/<g\b[^>]*fill="#AB12CD"[^>]*>\s*<path\b/i', $coloredSvg)) {
        $failures[] = 'title glyph path color';
    }
    if (str_replace('#AB12CD', '#135724', $coloredSvg) !== $plainSvg) {
        $failures[] = 'only title path paint changes';
    }
    $plain = imagecreatefromstring($plainPng);
    $colored = imagecreatefromstring($coloredPng);
    $changed = 0;
    $titleInk = 0;
    $outsideChanged = 0;
    for ($y = 0; $y < imagesy($plain); $y++) {
        for ($x = 0; $x < imagesx($plain); $x++) {
            $before = imagecolorat($plain, $x, $y);
            $after = imagecolorat($colored, $x, $y);
            if ($before !== $after) {
                $changed++;
                // The 18pt title starts at y=12; 60 includes its antialiased edge.
                if ($y >= 60) $outsideChanged++;
            }
            if ($y < 60 && ($after & 0xFFFFFF) === 0xAB12CD) $titleInk++;
        }
    }
    // Require real title ink, but do not pin glyph counts to a particular font.
    if ($changed < 10 || $titleInk < 10) $failures[] = 'PNG title color';
    if ($outsideChanged !== 0) $failures[] = 'non-title pixels changed';
    unset($plain, $colored);
    if ($chart->renderSvg() !== $coloredSvg || $chart->renderPng() !== $coloredPng) {
        $failures[] = 'repeated render';
    }
    $chart->setTitleColor(-1);
    if ($chart->renderSvg() !== $plainSvg || $chart->renderPng() !== $plainPng) {
        $failures[] = 'reset to text';
    }
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
ViolinPlot: OK
WordCloud: OK
