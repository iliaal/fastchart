--TEST--
Opposite extreme finite values retain distinct axis positions
--EXTENSIONS--
fastchart
--FILE--
<?php

$M = 1.7e308;
$svg = (new FastChart\LineChart(400, 300))
	->setSeries([-$M, 0, $M])
	->renderSvg();

preg_match('/<polyline points="([^"]+)"/', $svg, $match);
$points = array_map(
	fn(string $point): array => array_map('intval', explode(',', $point)),
	preg_split('/\s+/', trim($match[1] ?? ''))
);
$ys = array_column($points, 1);
echo 'line_has_three_points: ', count($ys) === 3 ? "yes\n" : "NO\n";
echo 'line_extremes_ordered: ',
	count($ys) === 3 && $ys[0] > $ys[1] && $ys[1] > $ys[2] ? "yes\n" : "NO\n";

$scatter = (new FastChart\ScatterChart(240, 160))
	->setPoints([
		[-PHP_FLOAT_MAX, 0, 'href' => '/left'],
		[ PHP_FLOAT_MAX, 1, 'href' => '/right'],
	]);
$scatter->renderSvg();
$areas = $scatter->getImageMapAreas();
echo 'scatter_extremes_separated: ',
	$areas[0]['coords'][0] + 100 < $areas[1]['coords'][0] ? "yes\n" : "NO\n";

$vector = (new FastChart\VectorChart(320, 240))
	->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
	->setVectors([
		['x' => -$M, 'y' => -$M, 'dx' => 1, 'dy' => 0],
		['x' =>  $M, 'y' =>  $M, 'dx' => 1, 'dy' => 0],
	])
	->renderSvg();
echo 'vector_finite: ',
	stripos($vector, 'nan') === false && stripos($vector, 'inf') === false
		? "yes\n" : "NO\n";

$forced = (new FastChart\LineChart(400, 300))
	->setSeries([-$M, 0, $M])
	->setYAxisRange(-$M, $M, 1.0e307)
	->renderSvg();
echo 'forced_interval_finite: ',
	stripos($forced, 'nan') === false && stripos($forced, 'inf') === false
		? "yes\n" : "NO\n";

foreach ([$M, -$M] as $equal) {
	$equalSvg = (new FastChart\LineChart(400, 300))
		->setSeries([$equal, $equal])
		->renderSvg();
	echo $equal > 0 ? 'equal_positive_finite: ' : 'equal_negative_finite: ';
	echo stripos($equalSvg, 'nan') === false
		&& stripos($equalSvg, 'inf') === false ? "yes\n" : "NO\n";
}

?>
--EXPECT--
line_has_three_points: yes
line_extremes_ordered: yes
scatter_extremes_separated: yes
vector_finite: yes
forced_interval_finite: yes
equal_positive_finite: yes
equal_negative_finite: yes
