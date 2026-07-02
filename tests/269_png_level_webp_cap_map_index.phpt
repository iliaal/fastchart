--TEST--
setPngCompressionLevel; WebP 16383 pre-raster reject; image-map original indexes
--EXTENSIONS--
fastchart
gd
--SKIPIF--
<?php
if (!function_exists('imagecreatefromstring')) die('skip gd without PNG support');
?>
--FILE--
<?php

use FastChart\BarChart;
use FastChart\LineChart;

function chart(): LineChart {
    return (new LineChart(400, 300))->setSeries([1, 5, 3, 8, 2]);
}

/* Level 0 (store) must be larger than level 9 (max), and both must
 * decode to the same pixels as the default. */
$def = chart()->renderPng();
$lo  = chart()->setPngCompressionLevel(0)->renderPng();
$hi  = chart()->setPngCompressionLevel(9)->renderPng();
echo "level0_larger: ", (strlen($lo) > strlen($hi) ? 'yes' : 'NO'), "\n";

$px = function (string $png): string {
    $im = imagecreatefromstring($png);
    $h = hash_init('sha1');
    for ($y = 0; $y < imagesy($im); $y += 7) {
        for ($x = 0; $x < imagesx($im); $x += 7) {
            hash_update($h, (string)imagecolorat($im, $x, $y));
        }
    }
    return hash_final($h);
};
echo "pixels_identical: ", ($px($def) === $px($lo) && $px($def) === $px($hi) ? 'yes' : 'NO'), "\n";

foreach ([-1, 10] as $bad) {
    try {
        chart()->setPngCompressionLevel($bad);
        echo "level_$bad: NO THROW\n";
    } catch (ValueError $e) {
        echo "level_$bad: throws\n";
    }
}

/* WebP rejects >16383 physical pixels per side up front; PNG at the
 * same size still renders (16384 is fastchart's own per-axis cap). */
$wide = (new LineChart(16384, 50))->setSeries([1, 2, 3]);
try {
    $wide->renderWebp();
    echo "webp_cap: NO THROW\n";
} catch (Error $e) {
    echo "webp_cap: throws (", str_contains($e->getMessage(), '16383') ? 'ok' : 'BAD MSG', ")\n";
}
$png = $wide->renderPng();
echo "png_same_size_ok: ", (substr($png, 1, 3) === 'PNG' ? 'yes' : 'NO'), "\n";

/* getImageMapAreas 'index' reports the position in the original
 * series, not the area slot ordinal: entry 1 has no href and is
 * skipped, so the second area must carry index 2, not 1. */
$bar = (new BarChart(400, 300))
    ->setSeries([10, 20, 30])
    ->setImageMap([
        ['href' => '/a', 'tooltip' => 'A'],
        ['tooltip' => 'no link'],
        ['href' => '/c'],
    ]);
$bar->renderSvg();
$idx = array_map(fn($a) => $a['index'], $bar->getImageMapAreas());
echo "map_orig_indexes: ", (implode(',', $idx) === '0,2' ? 'yes' : implode(',', $idx)), "\n";

?>
--EXPECT--
level0_larger: yes
pixels_identical: yes
level_-1: throws
level_10: throws
webp_cap: throws (ok)
png_same_size_ok: yes
map_orig_indexes: yes
