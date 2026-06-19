--TEST--
New charts: non-finite / overflow / degenerate input guards
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* No layout path may emit a non-finite or INT_MIN coordinate, whatever
 * the input. Each case below fed garbage-but-finite extremes that
 * previously reached an int cast as undefined behavior. */
function clean(string $svg): bool {
    foreach (['-2147483648', 'NaN', 'nan', 'inf', '="-2', '="-1.#'] as $bad) {
        if (strpos($svg, $bad) !== false) return false;
    }
    return simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

/* CR-001: ViolinPlot with finite-but-enormous samples. All values are
 * beyond the magnitude cap, so they drop and the chart has no data. */
try {
    (new FastChart\ViolinPlot(400, 300))
        ->setGroups([['label' => 'X', 'values' => [1e308, -1e308, 2e308, 5e307]]])
        ->renderSvg();
    echo "violin_huge: no_throw\n";
} catch (\Throwable $e) {
    echo "violin_huge: threw\n";
}
/* Mixed: the huge value drops, the finite ones render cleanly. */
$v = (new FastChart\ViolinPlot(400, 300))
    ->setGroups([['label' => 'X', 'values' => [1.0, 2.0, 3.0, 1e308, 4.0, 5.0]]])
    ->renderSvg();
echo "violin_mixed_clean: ", clean($v) ? "yes" : "no", "\n";

/* CR-002: ChordDiagram with huge duplicate links. */
$c = (new FastChart\ChordDiagram(400, 400))
    ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
    ->setLinks([
        ['from' => 0, 'to' => 1, 'value' => 1e308],   /* dropped */
        ['from' => 1, 'to' => 2, 'value' => 5],
        ['from' => 2, 'to' => 0, 'value' => 3],
    ])
    ->renderSvg();
echo "chord_huge_clean: ", clean($c) ? "yes" : "no", "\n";

/* CR-004: CirclePacking with a huge leaf value. */
$p = (new FastChart\CirclePacking(400, 400))
    ->setHierarchy(['children' => [['value' => 1e308], ['value' => 5], ['value' => 8]]])
    ->renderSvg();
echo "circlepack_huge_clean: ", clean($p) ? "yes" : "no", "\n";

/* CR-003: CirclePacking child array far larger than the node cap must
 * not allocate beyond the cap (and must still render). */
$kids = [];
for ($i = 0; $i < 50000; $i++) { $kids[] = ['value' => 1]; }
$p2 = (new FastChart\CirclePacking(300, 300))->setHierarchy(['children' => $kids])->renderSvg();
echo "circlepack_bigarray_clean: ", clean($p2) ? "yes" : "no", "\n";

/* CR-005: VennDiagram on a canvas too small to draw into renders blank,
 * not a negative radius. */
$vn = (new FastChart\VennDiagram(20, 20))
    ->setSets([['size' => 10], ['size' => 10]])
    ->setIntersections([['sets' => [0, 1], 'size' => 4]])
    ->renderSvg();
echo "venn_tiny_clean: ", clean($vn) ? "yes" : "no", "\n";
echo "venn_tiny_blank: ", (substr_count($vn, '<circle') === 0 ? "yes" : "no"), "\n";

/* CR-007: duplicate / reversed Venn pairs collapse to one (last wins). */
$vd = (new FastChart\VennDiagram(400, 400))
    ->setSets([['size' => 10], ['size' => 10], ['size' => 10]])
    ->setIntersections([
        ['sets' => [0, 1], 'size' => 3],
        ['sets' => [1, 0], 'size' => 5],   /* same pair, reversed */
        ['sets' => [0, 1], 'size' => 2],   /* same pair again */
    ])
    ->renderSvg();
echo "venn_dedup_clean: ", clean($vd) ? "yes" : "no", "\n";

/* CR-009: a ViolinPlot group whose values are all non-finite is dropped,
 * not kept as a blank column. One good group => one violin (2 polygons). */
$ve = (new FastChart\ViolinPlot(400, 300))
    ->setGroups([
        ['label' => 'good', 'values' => [1, 2, 3, 4, 3, 2]],
        ['label' => 'empty', 'values' => [INF, NAN, -INF]],
    ])
    ->renderSvg();
echo "violin_empty_dropped: ", (substr_count($ve, '<polygon') === 2 ? "yes" : "no"), "\n";

/* CR-010: only the fractional boundary icon gets a clip path; full icons
 * are drawn directly. 3.5 of 10 => 3 full + 1 partial => 1 clip. */
$pg = (new FastChart\Pictogram(400, 200))
    ->setTotal(10)->setValue(3.5)->setIconCount(10)
    ->setShape(FastChart\Pictogram::SHAPE_SQUARE)
    ->renderSvg();
echo "pictogram_one_clip: ", (substr_count($pg, '<clipPath') === 1 ? "yes" : "no"), "\n";

/* CR-006: a leaf with zero/missing value carries no area and is dropped,
 * not drawn as a min-radius placeholder dot. Of three children only the
 * valued one survives, so the root (1 outline circle) wraps a single
 * leaf (fill + border = 2 circles) => 3 circles total. */
$cp = (new FastChart\CirclePacking(300, 300))
    ->setHierarchy(['children' => [
        ['value' => 0],     /* zero => dropped */
        ['value' => 7],     /* kept */
        ['label' => 'x'],   /* no value => dropped */
    ]])
    ->renderSvg();
echo "circlepack_zero_leaf_dropped: ",
    (substr_count($cp, '<circle') === 3 ? "yes" : "no"), "\n";

/* CR-006: when every leaf prunes away the hierarchy is empty and draw
 * throws rather than emitting a lone placeholder circle. */
try {
    (new FastChart\CirclePacking(300, 300))
        ->setHierarchy(['children' => [['value' => 0], ['value' => -5]]])
        ->renderSvg();
    echo "circlepack_empty_throws: no_throw\n";
} catch (\Throwable $e) {
    echo "circlepack_empty_throws: threw\n";
}

/* CR-007: a geometrically impossible overlap (larger than the smaller
 * set) is dropped, not saturated to full containment. Dropped lays the
 * two equal circles side by side; a valid full-containment overlap makes
 * them concentric. The two layouts differ. */
$venn_mk = function (float $isize) {
    return (new FastChart\VennDiagram(400, 400))
        ->setSets([['size' => 10], ['size' => 10]])
        ->setIntersections([['sets' => [0, 1], 'size' => $isize]])
        ->renderSvg();
};
$venn_impossible = $venn_mk(100.0);   /* > min set size => dropped */
$venn_full       = $venn_mk(10.0);    /* == min set size => full overlap */
echo "venn_impossible_dropped: ",
    ($venn_impossible !== $venn_full ? "yes" : "no"), "\n";
echo "venn_impossible_clean: ", clean($venn_impossible) ? "yes" : "no", "\n";

echo "ok\n";
?>
--EXPECT--
violin_huge: threw
violin_mixed_clean: yes
chord_huge_clean: yes
circlepack_huge_clean: yes
circlepack_bigarray_clean: yes
venn_tiny_clean: yes
venn_tiny_blank: yes
venn_dedup_clean: yes
violin_empty_dropped: yes
pictogram_one_clip: yes
circlepack_zero_leaf_dropped: yes
circlepack_empty_throws: threw
venn_impossible_dropped: yes
venn_impossible_clean: yes
ok
