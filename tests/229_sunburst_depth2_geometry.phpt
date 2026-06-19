--TEST--
SunburstChart: depth-2 hierarchy partitions wedges by direct children
--EXTENSIONS--
fastchart
gd
--SKIPIF--
<?php
if (!function_exists('imagecreatefromstring')) die('skip gd required');
?>
--FILE--
<?php

/* Regression: the DFS builder recorded children as a contiguous
 * [child_first, +child_count) range, but a child's whole subtree is
 * appended before its next sibling, so direct children are interleaved
 * with grandchildren. Wedge geometry and value sums then read the wrong
 * nodes: later top-level siblings collapsed to zero-width slivers.
 *
 * Root has three equal-valued top-level branches (A=20 via A1+A2, B=20,
 * C=20). Correct output gives each a 120-degree inner wedge around the
 * full circle. The bug collapsed B and C, leaving the inner ring's
 * colored pixels confined to roughly one wedge. Probe the inner ring by
 * sampling a circle of points and counting how many distinct quadrants
 * carry non-background pixels: a correct three-way split lights up all
 * four quadrants; the collapsed version does not. */

$c = (new FastChart\SunburstChart(420, 420))
    ->setHierarchy([
        'label' => 'root', 'children' => [
            ['label' => 'A', 'color' => 0x6C8EBF, 'children' => [
                ['label' => 'A1', 'value' => 10], ['label' => 'A2', 'value' => 10]]],
            ['label' => 'B', 'value' => 20, 'color' => 0x82B366],
            ['label' => 'C', 'value' => 20, 'color' => 0xD79B00],
        ],
    ]);

$im = imagecreatefromstring($c->renderPng());
$w = imagesx($im); $h = imagesy($im);
$cx = $w / 2; $cy = $h / 2;
$r = min($w, $h) * 0.20;            // mid inner-ring (ring 1) radius
$quadrants = [];
for ($deg = 0; $deg < 360; $deg += 5) {
    $rad = deg2rad($deg);
    $x = (int)($cx + $r * cos($rad));
    $y = (int)($cy + $r * sin($rad));
    $rgb = imagecolorat($im, $x, $y);
    $rr = ($rgb >> 16) & 0xFF; $gg = ($rgb >> 8) & 0xFF; $bb = $rgb & 0xFF;
    if (!($rr > 240 && $gg > 240 && $bb > 240)) {
        $quadrants[intdiv($deg, 90)] = true;
    }
}
echo "inner_ring_spans_all_quadrants: ", (count($quadrants) === 4) ? "yes" : "no", "\n";
echo "ok\n";
?>
--EXPECT--
inner_ring_spans_all_quadrants: yes
ok
