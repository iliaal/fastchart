<?php
/* WordCloud differential harness: one line per render, "<name>\t<md5 of
 * the SVG>". Run it against two builds and diff the results:
 *
 *   php -d extension=/path/base.so bench/wordcloud_equivalence.php > base.txt
 *   php -d extension=/path/head.so bench/wordcloud_equivalence.php > head.txt
 *   diff base.txt head.txt
 *
 * Covers 1-256 words, canvases from 120x90 to 1600x1200, custom plot
 * rectangles, titles, both orientations, equal and spread weights,
 * words too large for the plot, and word lists that exhaust the shared
 * 2,000,000-step placement budget. Optional args: seed count, font path
 * (default: the first test-suite font candidate). */

if (!extension_loaded('fastchart')) {
    fwrite(STDERR, "fastchart extension not loaded\n");
    exit(1);
}
require __DIR__ . '/../tests/_font_candidates.inc';
$N = (int)($argv[1] ?? 300);
$font = $argv[2] ?? fc_pick_font();
if ($font === '') {
    fwrite(STDERR, "no font\n");
    exit(1);
}

$sizes = [[120, 90], [200, 150], [300, 200], [400, 300], [600, 400],
          [800, 600], [1000, 800], [1600, 1200], [300, 900], [900, 250]];
$orients = [FastChart\WordCloud::ORIENT_HORIZONTAL,
            FastChart\WordCloud::ORIENT_MIXED];

for ($seed = 1; $seed <= $N; $seed++) {
    mt_srand($seed);
    $n = [1, 3, 12, 40, 100, 180, 256][$seed % 7];
    $equal = $seed % 11 === 0;
    $words = [];
    for ($i = 0; $i < $n; $i++) {
        $len = $seed % 13 === 0 ? mt_rand(20, 80) : mt_rand(1, 14);
        $text = '';
        for ($k = 0; $k < $len; $k++) $text .= chr(mt_rand(0, 5) === 0 ? mt_rand(65, 90) : mt_rand(97, 122));
        $words[] = ['text' => $text, 'weight' => $equal ? 7 : mt_rand(1, $seed % 2 ? 100 : 100000)];
    }
    [$w, $h] = $sizes[$seed % count($sizes)];
    $chart = (new FastChart\WordCloud($w, $h))
        ->setFontPath($font)
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setOrientation($orients[$seed % 2])
        ->setWords($words);
    if ($seed % 5 === 0) $chart->setTitle('Title ' . $seed);
    if ($seed % 7 === 3) {
        $x0 = mt_rand(0, intdiv($w, 3));
        $y0 = mt_rand(0, intdiv($h, 3));
        $chart->setPlotRect($x0, $y0, mt_rand($x0 + 20, $w), mt_rand($y0 + 20, $h));
    }
    echo "seed $seed {$w}x{$h} n=$n\t", md5($chart->renderSvg()), "\n";
}
