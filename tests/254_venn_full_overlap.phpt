--TEST--
VennDiagram: three fully-overlapping sets render concentric, not as a separated triad
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_585c6812: a zero solved distance (full overlap) failed the d01 > 1e-6
 * gate and fell through to the symmetric-separated fallback, drawing
 * fully-overlapping sets apart. The degenerate case is now co-located. */

use FastChart\VennDiagram;

$svg = (new VennDiagram())->setSize(400, 400)
    ->setSets([
        ['label' => 'A', 'size' => 100],
        ['label' => 'B', 'size' => 100],
        ['label' => 'C', 'size' => 100],
    ])
    ->setIntersections([
        ['sets' => [0, 1], 'size' => 100],
        ['sets' => [0, 2], 'size' => 100],
        ['sets' => [1, 2], 'size' => 100],
    ])
    ->renderSvg();

echo "renders: ", (strlen($svg) > 500 ? 'yes' : 'no'), "\n";

/* The three set circles should be near-coincident (concentric), not spread
 * into a triad. Compare the bounding spread of ellipse centres. */
preg_match_all('/<ellipse[^>]*cx="(-?\d+(?:\.\d+)?)"[^>]*cy="(-?\d+(?:\.\d+)?)"/', $svg, $m);
$xs = array_map('floatval', $m[1]);
$ys = array_map('floatval', $m[2]);
$spread = (count($xs) >= 2)
    ? max(max($xs) - min($xs), max($ys) - min($ys))
    : 0;
echo "concentric: ", ($spread < 30 ? 'yes' : 'no'), "\n";

?>
--EXPECT--
renders: yes
concentric: yes
