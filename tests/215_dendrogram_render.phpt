--TEST--
Dendrogram: node-link hierarchy tree
--EXTENSIONS--
fastchart
simplexml
gd
--FILE--
<?php

$h = ['label' => 'root', 'children' => [
    ['label' => 'A', 'children' => [
        ['label' => 'a1', 'value' => 1], ['label' => 'a2', 'value' => 1]]],
    ['label' => 'B', 'children' => [
        ['label' => 'b1', 'value' => 1], ['label' => 'b2', 'value' => 1],
        ['label' => 'b3', 'value' => 1]]],
]];

$svg = (new FastChart\Dendrogram(500, 400))->setHierarchy($h)->renderSvg();
echo "well_formed: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false ? "yes" : "no"), "\n";
echo "has_edges: ",
    ((substr_count($svg, '<line') > 0 || substr_count($svg, '<polyline') > 0) ? "yes" : "no"), "\n";
echo "has_nodes: ", (substr_count($svg, '<circle') > 0 ? "yes" : "no"), "\n";

/* Elbow style + left orientation render cleanly (no overflow garbage). */
$svg2 = (new FastChart\Dendrogram(500, 400))->setHierarchy($h)
    ->setStyle(FastChart\Dendrogram::STYLE_ELBOW)
    ->setOrientation(FastChart\Dendrogram::ORIENT_LEFT)->renderSvg();
echo "elbow_left_clean: ",
    (simplexml_load_string($svg2, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false
        && strpos($svg2, '-2147483648') === false ? "yes" : "no"), "\n";

/* No hierarchy => draw throws. */
try {
    (new FastChart\Dendrogram(300, 300))->renderSvg();
    echo "empty: no_throw\n";
} catch (\Throwable $e) {
    echo "empty: threw\n";
}

/* Over-depth hierarchy rejected at the setter. */
$deep = ['value' => 1];
for ($i = 0; $i < 30; $i++) { $deep = ['children' => [$deep]]; }
try {
    (new FastChart\Dendrogram(300, 300))->setHierarchy($deep);
    echo "overdepth: no_throw\n";
} catch (\Throwable $e) {
    echo "overdepth: threw\n";
}

/* Raster round-trip (no imagedestroy — deprecated in 8.5). */
$im = imagecreatefromstring(
    (new FastChart\Dendrogram(400, 300))->setHierarchy($h)->renderPng());
echo "png_ok: ", ($im !== false ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
well_formed: yes
has_edges: yes
has_nodes: yes
elbow_left_clean: yes
empty: threw
overdepth: threw
png_ok: yes
ok
