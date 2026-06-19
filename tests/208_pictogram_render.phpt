--TEST--
Pictogram: fractional icon fill via clip
--EXTENSIONS--
fastchart
simplexml
gd
--FILE--
<?php

/* 7.5 of 10 person icons: 7 full + 1 partial + 2 empty. Full icons are
 * drawn directly (no clip); only the fractional boundary icon is clipped.
 * Each person = head circle + body polygon. So:
 *   7 full (fill draw) + 1 partial (empty base + clipped fill) + 2 empty
 *   = 7 + 2 + 2 = 11 head circles, and exactly 1 clip path. */
$c = (new FastChart\Pictogram(500, 250))
    ->setTotal(10)->setValue(7.5)->setIconCount(10)
    ->setShape(FastChart\Pictogram::SHAPE_PERSON)
    ->setFillColor(0x2266cc);
$svg = $c->renderSvg();

echo "circles_eq_11: ", (substr_count($svg, '<circle') === 11 ? "yes" : "no"), "\n";
echo "clips_eq_1: ", (substr_count($svg, '<clipPath') === 1 ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* All filled: no clips at all (every icon is full). */
$svg2 = (new FastChart\Pictogram(300, 150))
    ->setTotal(4)->setValue(4)->setIconCount(4)
    ->setShape(FastChart\Pictogram::SHAPE_CIRCLE)
    ->renderSvg();
echo "all_filled_clips_eq_0: ", (substr_count($svg2, '<clipPath') === 0 ? "yes" : "no"), "\n";

/* Zero value: no fill clips. */
$svg3 = (new FastChart\Pictogram(300, 150))
    ->setTotal(10)->setValue(0)->setIconCount(5)
    ->renderSvg();
echo "zero_clips_eq_0: ", (substr_count($svg3, '<clipPath') === 0 ? "yes" : "no"), "\n";

/* Value over total clamps to full: every icon full, no clips. */
$svg4 = (new FastChart\Pictogram(300, 150))
    ->setTotal(5)->setValue(99)->setIconCount(5)
    ->setShape(FastChart\Pictogram::SHAPE_SQUARE)
    ->renderSvg();
echo "overfill_clips_eq_0: ", (substr_count($svg4, '<clipPath') === 0 ? "yes" : "no"), "\n";

/* A fractional value clips exactly the one boundary icon. */
$svg5 = (new FastChart\Pictogram(300, 150))
    ->setTotal(10)->setValue(2.5)->setIconCount(10)
    ->setShape(FastChart\Pictogram::SHAPE_SQUARE)
    ->renderSvg();
echo "fractional_clips_eq_1: ", (substr_count($svg5, '<clipPath') === 1 ? "yes" : "no"), "\n";

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
circles_eq_11: yes
clips_eq_1: yes
well_formed_xml: yes
all_filled_clips_eq_0: yes
zero_clips_eq_0: yes
overfill_clips_eq_0: yes
fractional_clips_eq_1: yes
no_total: threw
png_ok: yes
ok
