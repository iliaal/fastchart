--TEST--
PieChart: setImageMap / setExplode indices stay setSlices-aligned after Other folding
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_326f9b7a: setOtherThreshold compacts the drawn slices, but the
 * render loop indexed explode[] and the image-map entries by the
 * post-aggregation position, shifting every hot-spot / explode offset
 * onto the wrong slice. The original setSlices() index is now carried
 * through the aggregation ("Other" gets none of either). */

use FastChart\PieChart;

function pie(): PieChart {
    return (new PieChart(300, 300))
        ->setSlices(['A' => 5, 'B' => 50, 'C' => 45])
        ->setOtherThreshold(10.0);
}

/* Slice A (5%) folds into Other; drawn slices are [B, C, Other]. */
$p = pie()->setImageMap([
    ['href' => '/slice/a'],
    ['href' => '/slice/b'],
    ['href' => '/slice/c'],
]);
$p->renderSvg();
$map = $p->getImageMap('m');
echo "areas: ", substr_count($map, '<area'), "\n";
echo "b_href: ", str_contains($map, 'href="/slice/b"') ? 'yes' : 'NO', "\n";
echo "c_href: ", str_contains($map, 'href="/slice/c"') ? 'yes' : 'NO', "\n";
echo "folded_a_href_absent: ", str_contains($map, 'href="/slice/a"') ? 'NO' : 'yes', "\n";

/* Explode keyed to the folded slice A must be a no-op (A isn't drawn);
 * keyed to B it must move slice B. */
$plain      = pie()->renderSvg();
$explode_a  = pie()->setExplode([0 => 30])->renderSvg();
$explode_b  = pie()->setExplode([1 => 30])->renderSvg();
echo "explode_on_folded_slice_noop: ", ($explode_a === $plain ? 'yes' : 'NO'), "\n";
echo "explode_on_kept_slice_moves: ", ($explode_b !== $plain ? 'yes' : 'NO'), "\n";

?>
--EXPECT--
areas: 2
b_href: yes
c_href: yes
folded_a_href_absent: yes
explode_on_folded_slice_noop: yes
explode_on_kept_slice_moves: yes
