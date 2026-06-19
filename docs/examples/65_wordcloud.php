<?php
/* WordCloud: each word's font size scales with its weight; words are
 * placed largest-first along a spiral, skipping any position that would
 * collide with an already-placed word. Layout is deterministic. Here,
 * tag frequency from an issue tracker. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\WordCloud(680, 420))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Issue tags by frequency')
    ->setWords([
        ['text' => 'rendering', 'weight' => 42, 'color' => 0x2266CC],
        ['text' => 'performance', 'weight' => 38],
        ['text' => 'svg', 'weight' => 31, 'color' => 0xD79B00],
        ['text' => 'memory', 'weight' => 28],
        ['text' => 'fonts', 'weight' => 24, 'color' => 0x82B366],
        ['text' => 'webp', 'weight' => 19],
        ['text' => 'api', 'weight' => 22],
        ['text' => 'docs', 'weight' => 16],
        ['text' => 'build', 'weight' => 14, 'color' => 0x9673A6],
        ['text' => 'tests', 'weight' => 18],
        ['text' => 'pdf', 'weight' => 11],
        ['text' => 'color', 'weight' => 13],
        ['text' => 'axis', 'weight' => 9],
        ['text' => 'legend', 'weight' => 7],
    ])
    ->renderToFile(__DIR__ . '/65_wordcloud.png');
