--TEST--
WordCloud charges every step of a word that runs out of reachable positions
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
require __DIR__ . '/_wordcloud_measure.inc';
$font = fc_pick_font();

/* One wide word is drawn at the plot centre, then copies of it follow.
 * The plot is narrower than three copies and shorter than three rows, so
 * every copy overlaps the first wherever it goes and walks all 60,000 of
 * its spiral steps, most of them past every in-bounds position. A narrow
 * word comes last and fits beside the first copy. After 33 copies
 * 2,000,000 - 1 - 33 * 60,000 = 19,999 steps remain, so it is drawn; the
 * 34th copy uses up the budget, so it is not. */
$big = ['text' => 'WWWWWWW', 'weight' => 10];
$small = ['text' => 'i', 'weight' => 10];
[$bw, $bh] = wc_box($font, 400, 300, $big, 10, 10);
[$sw, $sh] = wc_box($font, 400, 300, $small, 10, 10);
$pw = $bw + 2 * ($sw + 20);
$ph = max($bh, $sh) + 20;
echo 'copies cannot fit: ', (3 * $bw > $pw && 3 * $bh > $ph && $pw < 400 && $ph < 300) ? "yes\n" : "no\n";
$x0 = intdiv(400 - $pw, 2);
$y0 = intdiv(300 - $ph, 2);

foreach ([33, 34] as $copies) {
    $words = array_merge([$big], array_fill(0, $copies, $big), [$small]);
    $svg = (new FastChart\WordCloud(400, 300))
        ->setFontPath($font)
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setWords($words)
        ->setPlotRect($x0, $y0, $x0 + $pw, $y0 + $ph)
        ->renderSvg();
    preg_match_all('/<text\b[^>]*>([^<]*)<\/text>/', $svg, $m);
    echo "$copies copies: ", json_encode($m[1]), "\n";
}
?>
--EXPECT--
copies cannot fit: yes
33 copies: ["WWWWWWW","i"]
34 copies: ["WWWWWWW"]
