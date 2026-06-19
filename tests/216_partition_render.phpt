--TEST--
Partition: nested-rectangle hierarchy (partition + icicle orientations)
--EXTENSIONS--
fastchart
simplexml
gd
--FILE--
<?php

$hierarchy = ['children' => [
    ['label' => 'A', 'children' => [
        ['label' => 'a1', 'value' => 5],
        ['label' => 'a2', 'value' => 3],
    ]],
    ['label' => 'B', 'children' => [
        ['label' => 'b1', 'value' => 8],
        ['label' => 'b2', 'value' => 2],
        ['label' => 'b3', 'value' => 4],
    ]],
]];

$p = (new FastChart\Partition(500, 320))->setHierarchy($hierarchy);
$svg = $p->renderSvg();

echo "well_formed: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false
        ? "yes" : "no"), "\n";

/* Root + 2 internal + 5 leaves = 8 nodes, each a filled rect + a border rect.
 * At minimum every leaf draws a cell, so the rect count exceeds the 5 leaves. */
echo "has_cells: ", (substr_count($svg, '<rect') > 5 ? "yes" : "no"), "\n";

/* Icicle (vertical) orientation renders clean with no overflow coordinate. */
$ice = (new FastChart\Partition(500, 320))
    ->setOrientation(FastChart\Partition::ORIENT_VERTICAL)
    ->setHierarchy($hierarchy)->renderSvg();
echo "icicle_clean: ",
    (strpos($ice, '-2147483648') === false &&
     simplexml_load_string($ice, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false
        ? "yes" : "no"), "\n";

/* No hierarchy => draw throws. */
try {
    (new FastChart\Partition(300, 200))->renderSvg();
    echo "empty: no_throw\n";
} catch (\Throwable $e) {
    echo "empty: threw\n";
}

/* Over-depth hierarchy is rejected at the setter. */
$deep = ['value' => 1];
for ($i = 0; $i < 30; $i++) { $deep = ['children' => [$deep]]; }
try {
    (new FastChart\Partition(300, 200))->setHierarchy($deep);
    echo "overdepth: no_throw\n";
} catch (\Throwable $e) {
    echo "overdepth: threw\n";
}

/* Raster round-trip. */
$im = imagecreatefromstring($p->renderPng());
echo "png_ok: ", ($im !== false ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
well_formed: yes
has_cells: yes
icicle_clean: yes
empty: threw
overdepth: threw
png_ok: yes
ok
