--TEST--
WordCloud spiral reuse preserves deterministic layout bytes
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
$font = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf';
if (!is_file($font)) die('skip DejaVu Sans fixture font is unavailable');
?>
--FILE--
<?php

$font = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf';
$words = [];
for ($i = 0; $i < 256; $i++) {
    $words[] = ['text' => 'word' . $i, 'weight' => 256 - $i];
}

$render = static function () use ($font, $words): string {
    return (new FastChart\WordCloud(300, 200))
        ->setFontPath($font)
        ->setWords($words)
        ->setOrientation(FastChart\WordCloud::ORIENT_MIXED)
        ->renderSvg();
};

$first = $render();
$second = $render();

echo 'deterministic: ', $first === $second ? "yes\n" : "no\n";
echo 'layout_sha256: ', hash('sha256', $first), "\n";
?>
--EXPECT--
deterministic: yes
layout_sha256: b889482d47c3c6d9e1f2d866ad289c38f2df47cf355f0301c49305cc7ffebb94
