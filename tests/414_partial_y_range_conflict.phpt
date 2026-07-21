--TEST--
Conflicting partial Y ranges throw when the automatic endpoint is known
--EXTENSIONS--
fastchart
--FILE--
<?php

function render_outcome(FastChart\Chart $chart): string {
	try {
		$chart->renderSvg();
		return 'NO THROW';
	} catch (ValueError $e) {
		return str_contains($e->getMessage(), 'resolved min must be < resolved max')
			? 'ValueError' : 'WRONG MESSAGE';
	}
}

echo 'forced_min: ', render_outcome(
	(new FastChart\LineChart)->setSeries([1, 2, 3])->setYAxisRange(100, null)
), "\n";
echo 'forced_max: ', render_outcome(
	(new FastChart\LineChart)->setSeries([1, 2, 3])->setYAxisRange(null, -100)
), "\n";
echo 'waterfall_cleanup: ', render_outcome(
	(new FastChart\Waterfall)->setBars([
		['label' => 'A', 'value' => 1],
	])->setYAxisRange(100, null)
), "\n";

$recover = (new FastChart\LineChart)->setSeries([1, 2, 3])->setYAxisRange(100, null);
try {
	$recover->renderSvg();
} catch (ValueError $e) {
}
$recover->setYAxisRange(null, null);
echo 'reset_recovers: ', str_contains($recover->renderSvg(), '</svg>') ? "yes\n" : "NO\n";

echo 'valid_partial: ', str_contains(
	(new FastChart\LineChart)->setSeries([1, 2, 3])
		->setYAxisRange(0, null)->renderSvg(), '</svg>'
) ? "yes\n" : "NO\n";

$path = sys_get_temp_dir() . '/fastchart-partial-range-' . getmypid() . '.svg';
@unlink($path);
try {
	(new FastChart\LineChart)->setSeries([1, 2, 3])
		->setYAxisRange(100, null)->renderToFile($path);
} catch (ValueError $e) {
}
echo 'file_not_created: ', file_exists($path) ? "NO\n" : "yes\n";
@unlink($path);

?>
--EXPECT--
forced_min: ValueError
forced_max: ValueError
waterfall_cleanup: ValueError
reset_recovers: yes
valid_partial: yes
file_not_created: yes
