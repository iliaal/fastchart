--TEST--
PieChart: an exploded slice's image-map hot-spot tracks the offset slice, not its origin
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_fe341da5: the image-map polygon was recorded from the un-exploded centre
 * before setExplode() was applied, so the clickable area stayed over the
 * original wedge. The explode offset is now computed before the poly. */

use FastChart\PieChart;

function pie_map(bool $explode): string {
    $p = (new PieChart())->setSize(300, 300)
        ->setSlices(['A' => 30, 'B' => 50, 'C' => 20])
        ->setImageMap([['href' => '/a'], ['href' => '/b'], ['href' => '/c']]);
    if ($explode) {
        $p->setExplode([60, 0, 0]);
    }
    $p->renderSvg();
    return $p->getImageMap();
}

echo "map_tracks_explode: ", (pie_map(false) !== pie_map(true) ? 'yes' : 'no'), "\n";

?>
--EXPECT--
map_tracks_explode: yes
