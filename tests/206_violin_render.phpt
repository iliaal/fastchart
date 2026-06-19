--TEST--
ViolinPlot: KDE silhouettes with median ticks
--EXTENSIONS--
fastchart
simplexml
gd
--FILE--
<?php

/* Deterministic pseudo-samples (Box-Muller on a fixed seed). */
mt_srand(42);
function samples(float $mu, float $sd, int $n): array {
    $v = [];
    for ($i = 0; $i < $n; $i++) {
        $u1 = (mt_rand() + 1) / (mt_getrandmax() + 1);
        $u2 = (mt_rand() + 1) / (mt_getrandmax() + 1);
        $z = sqrt(-2 * log($u1)) * cos(2 * M_PI * $u2);
        $v[] = $mu + $sd * $z;
    }
    return $v;
}

$svg = (new FastChart\ViolinPlot(600, 400))
    ->setGroups([
        ['label' => 'A', 'color' => 0x44aa88, 'values' => samples(50, 10, 200)],
        ['label' => 'B', 'values' => samples(60, 5, 200)],
        ['label' => 'C', 'values' => samples(45, 15, 150)],
    ])
    ->renderSvg();

/* 3 groups => 3 fill + 3 stroke polygons, 3 median lines. */
echo "polygons_eq_6: ", (substr_count($svg, '<polygon') === 6 ? "yes" : "no"), "\n";
echo "median_lines_eq_3: ", (substr_count($svg, '<line') === 3 ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* Degenerate group (all identical values) still renders. */
$svg2 = (new FastChart\ViolinPlot(300, 300))
    ->setGroups([['label' => 'X', 'values' => [5, 5, 5, 5]]])
    ->renderSvg();
echo "degenerate_ok: ",
    (simplexml_load_string($svg2, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* Non-finite values are dropped, not fatal. */
$svg3 = (new FastChart\ViolinPlot(300, 300))
    ->setGroups([['label' => 'Y', 'values' => [1.0, 2.0, INF, NAN, 3.0, 4.0]]])
    ->renderSvg();
echo "nonfinite_dropped: ", (strlen($svg3) > 100 ? "yes" : "no"), "\n";

/* No groups is an error. */
try {
    (new FastChart\ViolinPlot(300, 200))->renderSvg();
    echo "empty: no_throw\n";
} catch (\Throwable $e) {
    echo "empty: threw\n";
}

/* Raster round-trip. */
$im = imagecreatefromstring(
    (new FastChart\ViolinPlot(320, 240))
        ->setGroups([['label' => 'Z', 'values' => [1, 2, 2, 3, 3, 3, 4, 4, 5]]])
        ->renderPng());
echo "png_ok: ", ($im !== false ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
polygons_eq_6: yes
median_lines_eq_3: yes
well_formed_xml: yes
degenerate_ok: yes
nonfinite_dropped: yes
empty: threw
png_ok: yes
ok
