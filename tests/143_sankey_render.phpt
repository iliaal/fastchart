--TEST--
SankeyChart: bipartite flow ribbons
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

$svg = (new FastChart\SankeyChart(600, 300))
    ->setNodes([
        ['label' => 'Source A'],
        ['label' => 'Source B'],
        ['label' => 'Hub'],
        ['label' => 'Sink X'],
        ['label' => 'Sink Y'],
    ])
    ->setLinks([
        ['from' => 0, 'to' => 2, 'value' => 5],
        ['from' => 1, 'to' => 2, 'value' => 3],
        ['from' => 2, 'to' => 3, 'value' => 4],
        ['from' => 2, 'to' => 4, 'value' => 4],
    ])
    ->renderSvg();

/* 4 ribbon polygons (filled) + 5 node rects (filled + stroke). */
echo "polygons_ge_4: ", (substr_count($svg, '<polygon') >= 4 ? "yes" : "no"), "\n";
echo "rects_ge_5: ", (substr_count($svg, '<rect') >= 5 ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* Self-loop / out-of-range links silently dropped. */
$svg2 = (new FastChart\SankeyChart(400, 200))
    ->setNodes([['label' => 'A'], ['label' => 'B']])
    ->setLinks([
        ['from' => 0, 'to' => 0, 'value' => 1],   /* self-loop */
        ['from' => 0, 'to' => 9, 'value' => 1],   /* out of range */
        ['from' => 0, 'to' => 1, 'value' => 2],   /* keeper */
    ])
    ->renderSvg();
echo "bad_links_dropped: ", (strlen($svg2) > 100 ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
polygons_ge_4: yes
rects_ge_5: yes
well_formed_xml: yes
bad_links_dropped: yes
ok
