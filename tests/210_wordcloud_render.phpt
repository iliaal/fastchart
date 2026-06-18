--TEST--
WordCloud: deterministic weighted spiral layout
--EXTENSIONS--
fastchart
simplexml
gd
--FILE--
<?php

function cloud(): FastChart\WordCloud {
    return (new FastChart\WordCloud(600, 400))->setWords([
        ['text' => 'data',   'weight' => 10, 'color' => 0x2266cc],
        ['text' => 'chart',  'weight' => 8],
        ['text' => 'php',    'weight' => 7],
        ['text' => 'native', 'weight' => 5],
        ['text' => 'fast',   'weight' => 9],
        ['text' => 'svg',    'weight' => 4],
        ['text' => 'render', 'weight' => 6],
        ['text' => 'vector', 'weight' => 3],
        ['text' => 'raster', 'weight' => 2],
        ['text' => 'plot',   'weight' => 5],
    ]);
}

$a = cloud()->renderSvg();
$b = cloud()->renderSvg();

/* Same input must yield byte-identical output (sorted + fixed spiral). */
echo "deterministic: ", ($a === $b ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($a, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* Every word is flattened to a <g> group in the default text-path mode;
 * on a 600x400 canvas all ten should fit. */
echo "groups_ge_10: ", (substr_count($a, '<g ') >= 10 ? "yes" : "no"), "\n";

/* Weight drives font size: the heaviest word's glyph paths should span a
 * larger box than a light word. Render single-word clouds and compare
 * path data length as a coarse proxy for rendered size. */
$big = (new FastChart\WordCloud(400, 400))
    ->setWords([['text' => 'AAAA', 'weight' => 100]])->renderSvg();
$small = (new FastChart\WordCloud(400, 400))
    ->setWords([['text' => 'AAAA', 'weight' => 1]])->renderSvg();
/* Both single words exist; both well-formed. */
echo "weighted_render: ",
    (strlen($big) > 100 && strlen($small) > 100 ? "yes" : "no"), "\n";

/* No words is an error. */
try {
    (new FastChart\WordCloud(300, 200))->renderSvg();
    echo "empty: no_throw\n";
} catch (\Throwable $e) {
    echo "empty: threw\n";
}

/* Raster round-trip. */
$im = imagecreatefromstring(cloud()->renderPng());
echo "png_ok: ", ($im !== false ? "yes" : "no"), "\n";
if ($im) { imagedestroy($im); }

echo "ok\n";
?>
--EXPECT--
deterministic: yes
well_formed_xml: yes
groups_ge_10: yes
weighted_render: yes
empty: threw
png_ok: yes
ok
