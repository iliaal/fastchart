--TEST--
Pictogram: fractional icon fill via clip
--EXTENSIONS--
fastchart
simplexml
gd
--FILE--
<?php

/* 7.5 of 10 person icons: 7 full + 1 partial filled, 2 empty.
 * Each person = head circle + body polygon. Base draws all 10;
 * the fill pass redraws 8 (clipped). */
$c = (new FastChart\Pictogram(500, 250))
    ->setTotal(10)->setValue(7.5)->setIconCount(10)
    ->setShape(FastChart\Pictogram::SHAPE_PERSON)
    ->setFillColor(0x2266cc);
$svg = $c->renderSvg();

echo "circles_eq_18: ", (substr_count($svg, '<circle') === 18 ? "yes" : "no"), "\n";   // 10 base + 8 filled heads
echo "clips_eq_8: ", (substr_count($svg, '<clipPath') === 8 ? "yes" : "no"), "\n";       // 7 full + 1 partial
echo "well_formed_xml: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* Circle shape, all filled. */
$svg2 = (new FastChart\Pictogram(300, 150))
    ->setTotal(4)->setValue(4)->setIconCount(4)
    ->setShape(FastChart\Pictogram::SHAPE_CIRCLE)
    ->renderSvg();
echo "all_filled_clips_eq_4: ", (substr_count($svg2, '<clipPath') === 4 ? "yes" : "no"), "\n";

/* Zero value: no fill clips. */
$svg3 = (new FastChart\Pictogram(300, 150))
    ->setTotal(10)->setValue(0)->setIconCount(5)
    ->renderSvg();
echo "zero_clips_eq_0: ", (substr_count($svg3, '<clipPath') === 0 ? "yes" : "no"), "\n";

/* Value over total clamps to full. */
$svg4 = (new FastChart\Pictogram(300, 150))
    ->setTotal(5)->setValue(99)->setIconCount(5)
    ->setShape(FastChart\Pictogram::SHAPE_SQUARE)
    ->renderSvg();
echo "overfill_clips_eq_5: ", (substr_count($svg4, '<clipPath') === 5 ? "yes" : "no"), "\n";

/* No total is an error. */
try {
    (new FastChart\Pictogram(200, 100))->setValue(1)->renderSvg();
    echo "no_total: no_throw\n";
} catch (\Throwable $e) {
    echo "no_total: threw\n";
}

/* Raster round-trip. */
$im = imagecreatefromstring(
    (new FastChart\Pictogram(240, 120))->setTotal(10)->setValue(3)->renderPng());
echo "png_ok: ", ($im !== false ? "yes" : "no"), "\n";
if ($im) { imagedestroy($im); }

echo "ok\n";
?>
--EXPECT--
circles_eq_18: yes
clips_eq_8: yes
well_formed_xml: yes
all_filled_clips_eq_4: yes
zero_clips_eq_0: yes
overfill_clips_eq_5: yes
no_total: threw
png_ok: yes
ok
