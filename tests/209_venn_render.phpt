--TEST--
VennDiagram: area-proportional circles with fitted overlap
--EXTENSIONS--
fastchart
simplexml
gd
--FILE--
<?php

/* Two equal sets (size 100) with intersection 30 => the overlap lens
 * should be ~30% of a circle's area. */
$svg = (new FastChart\VennDiagram(500, 400))
    ->setSets([
        ['label' => 'A', 'size' => 100, 'color' => 0xcc4444],
        ['label' => 'B', 'size' => 100, 'color' => 0x4444cc],
    ])
    ->setIntersections([['sets' => [0, 1], 'size' => 30]])
    ->renderSvg();

echo "circles_eq_4: ", (substr_count($svg, '<circle') === 4 ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* Pull the two stroke circles (fill="none") and measure the lens. */
preg_match_all('/<circle cx="([-\d.]+)" cy="([-\d.]+)" r="([-\d.]+)" fill="none"/',
               $svg, $m, PREG_SET_ORDER);
echo "stroke_circles_eq_2: ", (count($m) === 2 ? "yes" : "no"), "\n";

[$x0, $y0, $r0] = [(float)$m[0][1], (float)$m[0][2], (float)$m[0][3]];
[$x1, $y1, $r1] = [(float)$m[1][1], (float)$m[1][2], (float)$m[1][3]];
$d = hypot($x1 - $x0, $y1 - $y0);

function lens(float $r, float $R, float $d): float {
    if ($d >= $r + $R) return 0.0;
    if ($d <= abs($r - $R)) return M_PI * min($r, $R) ** 2;
    $d1 = ($d*$d - $R*$R + $r*$r) / (2*$d);
    $d2 = $d - $d1;
    return $r*$r*acos($d1/$r) - $d1*sqrt(max(0, $r*$r-$d1*$d1))
         + $R*$R*acos($d2/$R) - $d2*sqrt(max(0, $R*$R-$d2*$d2));
}
$ratio = lens($r0, $r1, $d) / (M_PI * $r0 * $r0);
echo "overlap_ratio_near_0.30: ", (abs($ratio - 0.30) < 0.05 ? "yes" : "no"), "\n";

/* Three sets render six circles. */
$svg3 = (new FastChart\VennDiagram(500, 500))
    ->setSets([['label' => 'X', 'size' => 80], ['label' => 'Y', 'size' => 60], ['label' => 'Z', 'size' => 50]])
    ->setIntersections([
        ['sets' => [0, 1], 'size' => 20],
        ['sets' => [0, 2], 'size' => 15],
        ['sets' => [1, 2], 'size' => 10],
    ])
    ->renderSvg();
echo "three_sets_circles_eq_6: ", (substr_count($svg3, '<circle') === 6 ? "yes" : "no"), "\n";

/* Fewer than 2 sets is an error. */
try {
    (new FastChart\VennDiagram(300, 300))->setSets([['label' => 'A', 'size' => 5]])->renderSvg();
    echo "one_set: no_throw\n";
} catch (\Throwable $e) {
    echo "one_set: threw\n";
}

/* Raster round-trip. */
$im = imagecreatefromstring(
    (new FastChart\VennDiagram(240, 200))
        ->setSets([['size' => 10], ['size' => 10]])
        ->setIntersections([['sets' => [0, 1], 'size' => 4]])
        ->renderPng());
echo "png_ok: ", ($im !== false ? "yes" : "no"), "\n";
if ($im) { imagedestroy($im); }

echo "ok\n";
?>
--EXPECT--
circles_eq_4: yes
well_formed_xml: yes
stroke_circles_eq_2: yes
overlap_ratio_near_0.30: yes
three_sets_circles_eq_6: yes
one_set: threw
png_ok: yes
ok
