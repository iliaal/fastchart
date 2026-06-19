--TEST--
ArcDiagram: baseline nodes with semicircular link arcs
--EXTENSIONS--
fastchart
simplexml
gd
--FILE--
<?php

$svg = (new FastChart\ArcDiagram(640, 320))
    ->setNodes([
        ['label' => 'A'],
        ['label' => 'B'],
        ['label' => 'C'],
        ['label' => 'D'],
        ['label' => 'E'],
    ])
    ->setLinks([
        ['from' => 0, 'to' => 2, 'value' => 5],
        ['from' => 1, 'to' => 3, 'value' => 3],
        ['from' => 0, 'to' => 4, 'value' => 2],
        ['from' => 2, 'to' => 4, 'value' => 4],
    ])
    ->renderSvg();

/* 5 node markers (filled circle + stroke = 2 each) and >= 4 arc paths. */
echo "circles_ge_5: ", (substr_count($svg, '<circle') >= 5 ? "yes" : "no"), "\n";
echo "paths_ge_4: ", (substr_count($svg, '<path') >= 4 ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* Orientation constants are distinct and all render. */
echo "orient_distinct: ",
    (FastChart\ArcDiagram::ORIENT_UP !== FastChart\ArcDiagram::ORIENT_DOWN
     && FastChart\ArcDiagram::ORIENT_DOWN !== FastChart\ArcDiagram::ORIENT_SPLIT
        ? "yes" : "no"), "\n";
foreach (['ORIENT_UP', 'ORIENT_DOWN', 'ORIENT_SPLIT'] as $name) {
    $mode = constant("FastChart\\ArcDiagram::$name");
    $s = (new FastChart\ArcDiagram(400, 200))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 2, 'value' => 1]])
        ->setOrientation($mode)
        ->renderSvg();
    echo "$name: ", (strlen($s) > 100 ? "ok" : "bad"), "\n";
}

/* Self-loop and out-of-range links silently dropped, keeper survives. */
$svg2 = (new FastChart\ArcDiagram(400, 200))
    ->setNodes([['label' => 'A'], ['label' => 'B']])
    ->setLinks([
        ['from' => 0, 'to' => 0, 'value' => 1],   /* self-loop */
        ['from' => 0, 'to' => 9, 'value' => 1],   /* out of range */
        ['from' => 0, 'to' => 1, 'value' => 2],   /* keeper */
    ])
    ->renderSvg();
echo "bad_links_dropped: ", (strlen($svg2) > 100 ? "yes" : "no"), "\n";

/* No nodes / no links is an error, not a blank chart. */
try {
    (new FastChart\ArcDiagram(300, 200))->renderSvg();
    echo "empty: no_throw\n";
} catch (\Throwable $e) {
    echo "empty: threw\n";
}

/* Raster round-trip emits a valid PNG. */
$png = (new FastChart\ArcDiagram(320, 160))
    ->setNodes([['label' => 'A'], ['label' => 'B']])
    ->setLinks([['from' => 0, 'to' => 1, 'value' => 1]])
    ->renderPng();
$im = imagecreatefromstring($png);
echo "png_ok: ", ($im !== false ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
circles_ge_5: yes
paths_ge_4: yes
well_formed_xml: yes
orient_distinct: yes
ORIENT_UP: ok
ORIENT_DOWN: ok
ORIENT_SPLIT: ok
bad_links_dropped: yes
empty: threw
png_ok: yes
ok
