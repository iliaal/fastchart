--TEST--
WordCloud places words exactly where a full scan of every spiral step does
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

/* Reference: the placement loop with every spiral step walked and every
 * placed box tested, 60,000 steps per word from a shared 2,000,000-step
 * budget. Returns the expected [text, x, y] of each drawn word in order. */
function wc_reference(array $words, array $boxes, array $rect, bool $mixed): array {
    [$px0, $py0, $px1, $py1] = $rect;
    $cx = ($px0 + $px1) / 2.0;
    $cy = ($py0 + $py1) / 2.0;
    $order = array_keys($words);
    usort($order, fn($a, $b) => $words[$b]['weight'] <=> $words[$a]['weight']);
    $placed = [];
    $out = [];
    $budget = 2000000;
    foreach ($order as $wi) {
        if ($boxes[$wi] === null) continue;
        [$tw, $th] = $boxes[$wi];
        $vertical = $mixed && $wi % 3 === 1;
        $bw = $vertical ? $th : $tw;
        $bh = $vertical ? $tw : $th;
        if ($bw > $px1 - $px0 || $bh > $py1 - $py0) continue;
        $hw = $bw / 2.0;
        $hh = $bh / 2.0;
        $ok = false;
        for ($s = 0; $s < 60000 && !$ok && $budget > 0; $s++, $budget--) {
            $t = (float)$s;
            $ang = $t * 2.39996322972865332;
            $rad = 1.5 * sqrt($t);
            $bx = $cx + $rad * cos($ang);
            $by = $cy + $rad * sin($ang);
            $x0 = $bx - $hw; $x1 = $bx + $hw; $y0 = $by - $hh; $y1 = $by + $hh;
            if ($x0 < $px0 || $x1 > $px1 || $y0 < $py0 || $y1 > $py1) continue;
            $ok = true;
            foreach ($placed as [$a0, $b0, $a1, $b1]) {
                if (!($x1 < $a0 - 1 || $x0 > $a1 + 1 || $y1 < $b0 - 1 || $y0 > $b1 + 1)) {
                    $ok = false;
                    break;
                }
            }
        }
        if (!$ok) {
            if ($budget <= 0) break;
            continue;
        }
        $placed[] = [$x0, $y0, $x1, $y1];
        $out[] = $vertical
            ? [$words[$wi]['text'], (int)($bx + $th * 0.35), (int)$by]
            : [$words[$wi]['text'], (int)$bx, (int)($by + $th * 0.35)];
    }
    return $out;
}

$dict = ['data', 'chart', 'render', 'layout', 'spiral', 'cloud', 'word',
         'weight', 'budget', 'canvas', 'glyph', 'pixel', 'vector', 'axis',
         'label', 'title', 'legend', 'series', 'value', 'index', 'cache',
         'memory', 'thread', 'queue', 'stream', 'buffer', 'parse', 'token',
         'syntax', 'server', 'client', 'session', 'header', 'json', 'array'];
/* [canvas w, h, plot rect, word count, mixed]: a wide plot, a tall mixed
 * plot, and a small plot where some words find no free position. */
$cases = [
    [400, 300, [10, 20, 390, 290], 35, false],
    [300, 420, [6, 6, 294, 414], 35, true],
    [360, 240, [100, 60, 260, 180], 30, true],
];
foreach ($cases as $i => [$w, $h, $rect, $n, $mixed]) {
    $words = [];
    for ($k = 0; $k < $n; $k++) {
        $words[] = ['text' => $dict[$k], 'weight' => intdiv(1000, $k + 1) + ($k * 7) % 5];
    }
    $weights = array_column($words, 'weight');
    $boxes = [];
    foreach ($words as $word) {
        $boxes[] = wc_box($font, $w, $h, $word, min($weights), max($weights));
    }
    $svg = (new FastChart\WordCloud($w, $h))
        ->setFontPath($font)
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setOrientation($mixed ? FastChart\WordCloud::ORIENT_MIXED
                                : FastChart\WordCloud::ORIENT_HORIZONTAL)
        ->setWords($words)
        ->setPlotRect(...$rect)
        ->renderSvg();
    preg_match_all('/<text x="(\d+)" y="(\d+)"[^>]*>([a-z]+)<\/text>/', $svg, $m, PREG_SET_ORDER);
    $drawn = array_map(fn($r) => [$r[3], (int)$r[1], (int)$r[2]], $m);
    $expected = wc_reference($words, $boxes, $rect, $mixed);
    echo "case $i: ", $drawn === $expected ? 'same' : 'DIFFERENT',
        $i === 2 ? (count($expected) < $n ? ', some words dropped' : ', all words drawn') : '', "\n";
    if ($drawn !== $expected) {
        echo json_encode(['drawn' => $drawn, 'expected' => $expected]), "\n";
    }
}
?>
--EXPECT--
case 0: same
case 1: same
case 2: same, some words dropped
