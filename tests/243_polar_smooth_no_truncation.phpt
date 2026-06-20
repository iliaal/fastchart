--TEST--
PolarChart: a long smooth series is not truncated at the raw-input point cap
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_811f7497: smooth mode expands each segment into 8 Catmull-Rom samples but
 * reused the 1024-entry raw-input cap as the output cap, silently dropping
 * samples past ~128 segments. The output buffer is now sized for the expansion. */

use FastChart\PolarChart;
use FastChart\Chart;

$pts = [];
for ($i = 0; $i < 200; $i++) {
    $pts[] = [$i * 1.8, 50 + ($i % 20)];
}
$svg = (new PolarChart())->setSize(400, 400)
    ->setSeries($pts)
    ->setInterpolation(Chart::INTERP_SMOOTH)
    ->setFilled(true)
    ->renderSvg();

preg_match_all('/points="([^"]*)"/', $svg, $m);
$max = 0;
foreach ($m[1] as $p) {
    $n = substr_count(trim($p), ' ') + 1;
    if ($n > $max) $max = $n;
}
/* 200 segments * 8 = 1600 samples; the old 1024 cap would truncate. */
echo "smooth_not_truncated: ", ($max > 1024 ? 'yes' : 'no'), "\n";

?>
--EXPECT--
smooth_not_truncated: yes
