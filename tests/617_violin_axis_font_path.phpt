--TEST--
ViolinPlot applies and clears axis font paths without using the data-label font
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die("skip no system font available\n");
?>
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';
function chart(string $font): FastChart\ViolinPlot {
    return (new FastChart\ViolinPlot(600, 400))->setGroups([
        ['label' => 'Category', 'values' => [1, 2, 3, 5]],
    ])->setFontPath($font)->setTitle('Title')->setTitleFont(fc_pick_font(), 16)
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
}
$font = fc_pick_font();
$base = chart(__FILE__)->renderSvg();
$real = chart($font)->renderSvg();
echo 'distinct fixtures: ', $base !== $real ? 'OK' : 'FAIL', "\n";
$chart = chart(__FILE__)->setAxisFont($font);
echo 'axis path: ', $chart->renderSvg() === $real ? 'OK' : 'FAIL', "\n";
$styled = chart(__FILE__)->setAxisFont($font, 21)->setAxisLabelColor(0xAB12CD)->setLabelFont(null, 30);
preg_match('/<text\b[^>]*>Title<\/text>/', $real, $beforeTitle);
preg_match('/<text\b[^>]*>Title<\/text>/', $styled->renderSvg(), $afterTitle);
echo 'title unchanged: ', count($beforeTitle) === 1 && $afterTitle === $beforeTitle ? 'OK' : 'FAIL', "\n";
echo 'clear axis path: ', $chart->setAxisFont('')->renderSvg() === $base ? 'OK' : 'FAIL', "\n";
$chart->setLabelFont($font);
echo 'label path ignored: ', $chart->renderSvg() === $base ? 'OK' : 'FAIL', "\n";
echo 'both paths: ', $chart->setAxisFont($font)->renderSvg() === $real ? 'OK' : 'FAIL', "\n";
echo 'clear label path: ', $chart->setLabelFont('')->renderSvg() === $real ? 'OK' : 'FAIL', "\n";
// A valid axis font must render actual FreeType glyphs even when the
// global font points at an existing non-font file.
$chart = chart(__FILE__)->setAxisFont($font);
echo 'axis native: ', $chart->renderSvg() === $real ? 'OK' : 'FAIL', "\n";
$paths = chart($font)->setSvgTextMode(FastChart\Chart::SVG_TEXT_PATHS)->renderSvg();
$bare = chart(__FILE__)->setSvgTextMode(FastChart\Chart::SVG_TEXT_PATHS)->renderSvg();
echo 'glyph fixtures: ', $paths !== $bare ? 'OK' : 'FAIL', "\n";
echo 'axis glyphs: ', $chart->setSvgTextMode(FastChart\Chart::SVG_TEXT_PATHS)->renderSvg() === $paths ? 'OK' : 'FAIL', "\n";
?>
--EXPECT--
distinct fixtures: OK
axis path: OK
title unchanged: OK
clear axis path: OK
label path ignored: OK
both paths: OK
clear label path: OK
axis native: OK
glyph fixtures: OK
axis glyphs: OK
