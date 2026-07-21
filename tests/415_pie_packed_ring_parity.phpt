--TEST--
Pie packed rings share flat-slice parsing and setSlices leaves ring mode
--EXTENSIONS--
fastchart
--FILE--
<?php

$packed = null;
try {
	$packed = (new FastChart\PieChart(300, 300))
		->setRings([[10, 20]])
		->renderSvg();
} catch (Throwable $e) {
}
$dicts = (new FastChart\PieChart(300, 300))
	->setRings([[
		['value' => 10],
		['value' => 20],
	]])
	->renderSvg();
echo 'packed_ring_renders: ',
	is_string($packed) && str_contains($packed, '</svg>') ? "yes\n" : "NO\n";
echo 'packed_matches_dicts: ', $packed === $dicts ? "yes\n" : "NO\n";

$chart = (new FastChart\PieChart(300, 300))
	->setRings([['old-a' => 1, 'old-b' => 1]]);
$old = $chart->renderSvg();
$new = $chart->setSlices(['new-a' => 3, 'new-b' => 1])->renderSvg();
$fresh = (new FastChart\PieChart(300, 300))
	->setSlices(['new-a' => 3, 'new-b' => 1])
	->renderSvg();
echo 'setSlices_leaves_rings: ', $new !== $old && $new === $fresh ? "yes\n" : "NO\n";

$flat = (new FastChart\PieChart(300, 300))->setSlices(['a' => 2, 'b' => 1]);
$before = $flat->renderSvg();
$after = $flat->setRings([])->renderSvg();
echo 'empty_rings_preserve_flat: ', $before === $after ? "yes\n" : "NO\n";

?>
--EXPECT--
packed_ring_renders: yes
packed_matches_dicts: yes
setSlices_leaves_rings: yes
empty_rings_preserve_flat: yes
