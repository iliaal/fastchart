--TEST--
ChordDiagram: directed style draws an arrowhead per link target
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* STYLE_DIRECTED renders the translucent ribbons of STYLE_RIBBON plus one
 * filled arrowhead triangle at each link's target endpoint. So a directed
 * render has strictly more <polygon> elements than the plain ribbon of the
 * same data (one extra polygon per link), and the arrowhead coordinate math
 * must stay finite. */

function valid(string $svg): bool {
    return strlen($svg) > 100 &&
        simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

$nodes = [['label' => 'A'], ['label' => 'B'], ['label' => 'C']];
$links = [
    ['from' => 0, 'to' => 1, 'value' => 5],
    ['from' => 1, 'to' => 2, 'value' => 3],
    ['from' => 2, 'to' => 0, 'value' => 4],
];

$mk = function (int $style) use ($nodes, $links) {
    return (new FastChart\ChordDiagram(400, 400))
        ->setNodes($nodes)->setLinks($links)
        ->setStyle($style)->renderSvg();
};

$ribbon   = $mk(FastChart\ChordDiagram::STYLE_RIBBON);
$directed = $mk(FastChart\ChordDiagram::STYLE_DIRECTED);

echo "ribbon_valid: ", valid($ribbon) ? "yes" : "no", "\n";
echo "directed_valid: ", valid($directed) ? "yes" : "no", "\n";
/* One arrowhead polygon per link on top of the ribbons. */
echo "arrowheads_added: ",
    (substr_count($directed, '<polygon') - substr_count($ribbon, '<polygon') === count($links)
        ? "yes" : "no"), "\n";
echo "directed_clean: ", (strpos($directed, '-2147483648') === false ? "yes" : "no"), "\n";

/* Out-of-range style falls back to the default ribbon. */
$bad = $mk(99);
echo "bad_style_falls_back: ",
    (substr_count($bad, '<polygon') === substr_count($ribbon, '<polygon') ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
ribbon_valid: yes
directed_valid: yes
arrowheads_added: yes
directed_clean: yes
bad_style_falls_back: yes
ok
