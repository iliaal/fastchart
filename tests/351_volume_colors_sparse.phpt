--TEST--
StockChart::setVolumeColors honors sparse integer keys without misaligning
--EXTENSIONS--
fastchart
--FILE--
<?php

/* setVolumeColors previously probed packed indexes 0..num_elements-1, so a
 * sparse map like [2 => color] (or array_filter output with holes) shifted
 * colors onto the wrong candle or dropped them. It now walks by integer key
 * through max(key)+1, mirroring PieChart::setExplode. */

use FastChart\StockChart;

function volume_svg(array $colors): string {
    $rows = [];
    for ($i = 0; $i < 5; $i++) {
        $rows[] = [1700000000 + $i * 86400, 100, 102, 99, 101, 1000];
    }
    return strtolower((new StockChart(900, 500))
        ->setOhlcv($rows)
        ->setVolumePane(true)
        ->setVolumeColors($colors)
        ->renderSvg());
}

// [2 => blue]: candle index 2 gets blue; the old walk (num_elements == 1,
// probe index 0) never reached key 2, so blue was absent.
$svg = volume_svg([2 => 0x0000FF]);
echo "sparse_index2_blue: ", (strpos($svg, '#0000ff') !== false ? "yes" : "no"), "\n";

// Hole-bearing array [0 => red, 2 => blue] (index 1 is a hole -> palette
// default). Both named colors must survive; the old walk lost key 2.
$svg = volume_svg([0 => 0xFF0000, 2 => 0x0000FF]);
echo "hole_red:  ", (strpos($svg, '#ff0000') !== false ? "yes" : "no"), "\n";
echo "hole_blue: ", (strpos($svg, '#0000ff') !== false ? "yes" : "no"), "\n";

// Packed array is unchanged.
$svg = volume_svg([0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xFF00FF]);
$all = true;
foreach (['#ff0000', '#00ff00', '#0000ff', '#ffff00', '#ff00ff'] as $hex) {
    if (strpos($svg, $hex) === false) $all = false;
}
echo "packed_all_present: ", ($all ? "yes" : "no"), "\n";

?>
--EXPECT--
sparse_index2_blue: yes
hole_red:  yes
hole_blue: yes
packed_all_present: yes
