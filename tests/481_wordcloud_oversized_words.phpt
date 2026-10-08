--TEST--
WordCloud: oversized words do not starve smaller words of placement attempts
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die('skip no system font is available');
?>
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';

/* 34 impossible words would consume all 2,000,000 placement attempts
 * at 60,000 attempts each, before the final small word is reached. */
$words = array_fill(0, 34, ['text' => str_repeat('W', 100), 'weight' => 10]);
$words[] = ['text' => 'fits', 'weight' => 1];

foreach ([FastChart\WordCloud::ORIENT_HORIZONTAL, FastChart\WordCloud::ORIENT_MIXED] as $orientation) {
    foreach ([false, true] as $customPlot) {
        $chart = (new FastChart\WordCloud(400, 300))
            ->setFontPath(fc_pick_font())
            ->setOrientation($orientation)
            ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
            ->setWords($words);
        if ($customPlot) $chart->setPlotRect(100, 100, 200, 200);
        $svg = $chart->renderSvg();
        preg_match_all('/<text\b[^>]*>([^<]*)<\/text>/', $svg, $matches);
        echo 'only fitting word: ', $matches[1] === ['fits'] ? "yes\n" : "no\n";
        echo 'repeatable: ', $svg === $chart->renderSvg() ? "yes\n" : "no\n";
    }
}
?>
--EXPECT--
only fitting word: yes
repeatable: yes
only fitting word: yes
repeatable: yes
only fitting word: yes
repeatable: yes
only fitting word: yes
repeatable: yes
