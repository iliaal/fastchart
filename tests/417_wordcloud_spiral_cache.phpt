--TEST--
WordCloud spiral reuse preserves deterministic layout bytes
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
$font = fc_pick_font();
$words = [];
for ($i = 0; $i < 256; $i++) {
    $words[] = ['text' => 'word' . $i, 'weight' => 256 - $i];
}

$render = static function () use ($font, $words): string {
    return (new FastChart\WordCloud(300, 200))
        ->setFontPath($font)
        ->setWords($words)
        ->setOrientation(FastChart\WordCloud::ORIENT_MIXED)
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->renderSvg();
};

$first = $render();
$second = $render();

echo 'deterministic: ', $first === $second ? "yes\n" : "no\n";
$valid = str_starts_with($first, '<?xml version="1.0"')
    && str_contains($first, 'viewBox="0 0 300 200"')
    && str_ends_with($first, "</svg>\n");
echo 'valid SVG: ', $valid ? "yes\n" : "no\n";

preg_match_all(
    '/<text x="([^"]+)" y="([^"]+)"[^>]*>(word([0-9]+))<\/text>/',
    $first,
    $matches,
    PREG_SET_ORDER
);
$placed = array_column($matches, 3);
$unique = count($placed) >= 3
    && count($placed) === count(array_unique($placed))
    && in_array('word0', $placed, true);
echo 'multiple unique placed words: ', $unique ? "yes\n" : "no\n";

$inBounds = true;
foreach ($matches as $match) {
    $x = (float)$match[1];
    $y = (float)$match[2];
    $index = (int)$match[4];
    if (!is_finite($x) || !is_finite($y)
        || $x < 0 || $x > 300 || $y < 0 || $y > 200
        || $index < 0 || $index >= 256) {
        $inBounds = false;
        break;
    }
}
echo 'placed words in bounds: ', $inBounds ? "yes\n" : "no\n";

$source = file_get_contents(__DIR__ . '/../fastchart_wordcloud.c');
$cacheContract = $source !== false
    && str_contains($source, 'wc_spiral_point *spiral')
    && str_contains($source, 'if (s == spiral_n)')
    && str_contains($source, 'spiral[s].dx');
echo 'spiral cache contract: ', $cacheContract ? "yes\n" : "no\n";
?>
--EXPECT--
deterministic: yes
valid SVG: yes
multiple unique placed words: yes
placed words in bounds: yes
spiral cache contract: yes
