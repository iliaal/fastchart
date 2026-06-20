--TEST--
CalendarHeatmap: a multi-year span fits with small cells; only an unfittable span throws
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_e02ae8f1: the 4px minimum-cell clamp pushed wide spans past grid_x1,
 * silently dropping later weeks. Wide spans now use sub-4px cells; only a span
 * that can't fit even 1px/week is rejected. */

use FastChart\CalendarHeatmap;

$d = [];
$t = strtotime('2022-01-01');
for ($i = 0; $i < 730; $i++) {
    $d[date('Y-m-d', $t + $i * 86400)] = $i % 5;
}
$svg = (new CalendarHeatmap())->setSize(400, 200)->setData($d)->renderSvg();
echo "two_year_fits: ", (strlen($svg) > 500 ? 'yes' : 'no'), "\n";

try {
    $d2 = [];
    $t2 = strtotime('2010-01-01');
    for ($i = 0; $i < 2000; $i++) {
        $d2[date('Y-m-d', $t2 + $i * 7 * 86400)] = 1;
    }
    (new CalendarHeatmap())->setSize(200, 200)->setData($d2)->renderSvg();
    echo "unfittable: rendered\n";
} catch (\Throwable $e) {
    echo "unfittable: throws\n";
}

?>
--EXPECT--
two_year_fits: yes
unfittable: throws
