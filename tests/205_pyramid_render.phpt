--TEST--
PopulationPyramid: diverging back-to-back horizontal bars
--EXTENSIONS--
fastchart
simplexml
gd
--FILE--
<?php

$svg = (new FastChart\PopulationPyramid(600, 400))
    ->setCategories(['0-9', '10-19', '20-29', '30-39', '40-49', '50+'])
    ->setLeftSeries(['label' => 'Male', 'color' => 0x3366cc, 'data' => [12, 18, 22, 20, 15, 10]])
    ->setRightSeries(['label' => 'Female', 'color' => 0xcc3366, 'data' => [11, 17, 21, 22, 16, 13]])
    ->renderSvg();

/* bg + 6 left bars + 6 left strokes + 6 right bars + 6 right strokes
 * + 2 legend swatches = 27 rects. */
echo "rects_eq_27: ", (substr_count($svg, '<rect') === 27 ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* One-sided pyramid is allowed. */
$svg2 = (new FastChart\PopulationPyramid(400, 300))
    ->setCategories(['A', 'B', 'C'])
    ->setLeftSeries(['data' => [1, 2, 3]])
    ->renderSvg();
echo "one_sided: ", (strlen($svg2) > 100 ? "ok" : "bad"), "\n";

/* Shorter data than categories renders the present rows only. */
$svg3 = (new FastChart\PopulationPyramid(400, 300))
    ->setCategories(['A', 'B', 'C', 'D'])
    ->setLeftSeries(['data' => [5, 3]])
    ->setRightSeries(['data' => [4, 6, 2, 1]])
    ->renderSvg();
echo "ragged_ok: ",
    (simplexml_load_string($svg3, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* No categories / no sides is an error. */
try {
    (new FastChart\PopulationPyramid(300, 200))->renderSvg();
    echo "empty: no_throw\n";
} catch (\Throwable $e) {
    echo "empty: threw\n";
}

/* Raster round-trip. */
$im = imagecreatefromstring(
    (new FastChart\PopulationPyramid(320, 200))
        ->setCategories(['A', 'B'])
        ->setLeftSeries(['data' => [1, 2]])
        ->setRightSeries(['data' => [2, 1]])
        ->renderPng());
echo "png_ok: ", ($im !== false ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
rects_eq_27: yes
well_formed_xml: yes
one_sided: ok
ragged_ok: yes
empty: threw
png_ok: yes
ok
