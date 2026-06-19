--TEST--
ChordDiagram: circular node arcs with bezier ribbons
--EXTENSIONS--
fastchart
simplexml
gd
--FILE--
<?php

$svg = (new FastChart\ChordDiagram(500, 500))
    ->setNodes([
        ['label' => 'A', 'color' => 0xcc4444],
        ['label' => 'B'],
        ['label' => 'C'],
        ['label' => 'D'],
    ])
    ->setLinks([
        ['from' => 0, 'to' => 1, 'value' => 8],
        ['from' => 0, 'to' => 2, 'value' => 4],
        ['from' => 1, 'to' => 2, 'value' => 3],
        ['from' => 2, 'to' => 3, 'value' => 6],
        ['from' => 3, 'to' => 0, 'value' => 5],
    ])
    ->setPadAngle(3.0)
    ->renderSvg();

/* 5 ribbon polygons, >= 4 node-band arc paths. */
echo "polygons_ge_5: ", (substr_count($svg, '<polygon') >= 5 ? "yes" : "no"), "\n";
echo "paths_ge_4: ", (substr_count($svg, '<path') >= 4 ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* Self-loop and out-of-range links dropped, keepers survive. */
$svg2 = (new FastChart\ChordDiagram(300, 300))
    ->setNodes([['label' => 'A'], ['label' => 'B']])
    ->setLinks([
        ['from' => 0, 'to' => 0, 'value' => 1],   /* self-loop */
        ['from' => 0, 'to' => 5, 'value' => 1],   /* out of range */
        ['from' => 0, 'to' => 1, 'value' => 2],   /* keeper */
    ])
    ->renderSvg();
echo "bad_links_dropped: ", (substr_count($svg2, '<polygon') === 1 ? "yes" : "no"), "\n";

/* Pad angle is clamped, not fatal. */
$svg3 = (new FastChart\ChordDiagram(300, 300))
    ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
    ->setLinks([['from' => 0, 'to' => 1, 'value' => 1], ['from' => 1, 'to' => 2, 'value' => 1]])
    ->setPadAngle(999.0)
    ->renderSvg();
echo "pad_clamped: ", (strlen($svg3) > 100 ? "yes" : "no"), "\n";

/* No nodes / links is an error. */
try {
    (new FastChart\ChordDiagram(300, 300))->renderSvg();
    echo "empty: no_throw\n";
} catch (\Throwable $e) {
    echo "empty: threw\n";
}

/* Raster round-trip emits a valid PNG. */
$png = (new FastChart\ChordDiagram(240, 240))
    ->setNodes([['label' => 'A'], ['label' => 'B']])
    ->setLinks([['from' => 0, 'to' => 1, 'value' => 1]])
    ->renderPng();
$im = imagecreatefromstring($png);
echo "png_ok: ", ($im !== false ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
polygons_ge_5: yes
paths_ge_4: yes
well_formed_xml: yes
bad_links_dropped: yes
pad_clamped: yes
empty: threw
png_ok: yes
ok
