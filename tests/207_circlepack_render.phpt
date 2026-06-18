--TEST--
CirclePacking: nested circles with non-overlapping siblings
--EXTENSIONS--
fastchart
simplexml
gd
--FILE--
<?php

$svg = (new FastChart\CirclePacking(500, 500))
    ->setHierarchy([
        'label' => 'root',
        'children' => [
            ['label' => 'A', 'color' => 0x4488cc, 'children' => [
                ['label' => 'a1', 'value' => 5],
                ['label' => 'a2', 'value' => 8],
                ['label' => 'a3', 'value' => 3],
            ]],
            ['label' => 'B', 'color' => 0xcc8844, 'children' => [
                ['label' => 'b1', 'value' => 10],
                ['label' => 'b2', 'value' => 4],
            ]],
            ['label' => 'C', 'value' => 6],
        ],
    ])
    ->renderSvg();

/* 6 leaves (fill + stroke = 2 each) + 3 internal outlines = 15 circles. */
echo "circles_eq_15: ", (substr_count($svg, '<circle') === 15 ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* Flat packing: all leaves are siblings and must not overlap. */
$flat = (new FastChart\CirclePacking(400, 400))
    ->setHierarchy(['children' => [
        ['value' => 1], ['value' => 4], ['value' => 9],
        ['value' => 16], ['value' => 2], ['value' => 7], ['value' => 3],
    ]])
    ->renderSvg();

preg_match_all('/<circle cx="([-\d.]+)" cy="([-\d.]+)" r="([-\d.]+)"([^>]*)>/',
               $flat, $m, PREG_SET_ORDER);
$leaves = [];
foreach ($m as $c) {
    if (strpos($c[4], 'fill="none"') === false) {
        $leaves[] = [(float)$c[1], (float)$c[2], (float)$c[3]];
    }
}
echo "leaf_circles_eq_7: ", (count($leaves) === 7 ? "yes" : "no"), "\n";

$overlaps = 0;
for ($i = 0; $i < count($leaves); $i++) {
    for ($j = $i + 1; $j < count($leaves); $j++) {
        $dx = $leaves[$i][0] - $leaves[$j][0];
        $dy = $leaves[$i][1] - $leaves[$j][1];
        $dist = sqrt($dx * $dx + $dy * $dy);
        $need = $leaves[$i][2] + $leaves[$j][2];
        if ($dist < $need - 1.5) { $overlaps++; }   /* 1.5px int-rounding slack */
    }
}
echo "sibling_overlaps_eq_0: ", ($overlaps === 0 ? "yes" : "no"), "\n";

/* Single-leaf root and empty cases. */
$single = (new FastChart\CirclePacking(200, 200))
    ->setHierarchy(['label' => 'solo', 'value' => 5])->renderSvg();
echo "single_ok: ", (substr_count($single, '<circle') >= 2 ? "yes" : "no"), "\n";

try {
    (new FastChart\CirclePacking(200, 200))->renderSvg();
    echo "empty: no_throw\n";
} catch (\Throwable $e) {
    echo "empty: threw\n";
}

/* Raster round-trip. */
$im = imagecreatefromstring(
    (new FastChart\CirclePacking(240, 240))
        ->setHierarchy(['children' => [['value' => 3], ['value' => 5]]])
        ->renderPng());
echo "png_ok: ", ($im !== false ? "yes" : "no"), "\n";
if ($im) { imagedestroy($im); }

echo "ok\n";
?>
--EXPECT--
circles_eq_15: yes
well_formed_xml: yes
leaf_circles_eq_7: yes
sibling_overlaps_eq_0: yes
single_ok: yes
empty: threw
png_ok: yes
ok
