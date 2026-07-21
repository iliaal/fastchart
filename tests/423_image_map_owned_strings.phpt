--TEST--
Image-map strings survive resets, replacement, cloning, and source destruction
--EXTENSIONS--
fastchart
--FILE--
<?php

function result(string $label, bool $ok): void {
	echo $label, ': ', $ok ? "yes\n" : "NO\n";
}

function barWithMap(array $map): FastChart\BarChart {
	return (new FastChart\BarChart(320, 220))
		->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
		->setSeries([['data' => [2, 4]]])
		->setImageMap($map);
}

$href = '/' . str_repeat('owned-', 8) . 'target?x=1&y=2';
$tooltip = 'tip <&" Ω';
$input = [
	['href' => $href, 'tooltip' => $tooltip],
	['href' => '/second', 'tooltip' => 'second'],
];
$chart = barWithMap($input);
$input[0]['href'] = '/mutated';
$input[0]['tooltip'] = 'mutated';
unset($input, $href, $tooltip);
$chart->renderSvg();
$areas = $chart->getImageMapAreas();
$html = $chart->getImageMap('owned');
result('source_released',
	$areas[0]['href'] === '/owned-owned-owned-owned-owned-owned-owned-owned-target?x=1&y=2'
	&& $areas[0]['tooltip'] === 'tip <&" Ω'
	&& str_contains($html, 'x=1&amp;y=2')
	&& str_contains($html, 'title="tip &lt;&amp;&quot; Ω"'));

$expected = $areas;
$repeated = true;
for ($i = 0; $i < 25; $i++) {
	$chart->renderSvg();
	if ($chart->getImageMapAreas() !== $expected) {
		$repeated = false;
		break;
	}
}
result('repeated_reset_repopulate', $repeated);

$clone = clone $chart;
$chart->setImageMap([['href' => '/replacement', 'tooltip' => 'new']]);
unset($chart);
gc_collect_cycles();
$clone->renderSvg();
result('base_clone_outlives_source', $clone->getImageMapAreas() === $expected);

$replacement = barWithMap([['href' => '/old'], ['href' => '/old-two']]);
$replacement->renderSvg();
$replacement->setImageMap([['href' => '/new'], ['href' => '/new-two']]);
$emptyAfterReplace = $replacement->getImageMapAreas() === [];
$replacement->renderSvg();
$replacedAreas = $replacement->getImageMapAreas();
$replacement->setImageMap([]);
result('map_replacement_resets_artifact',
	$emptyAfterReplace
	&& $replacedAreas[0]['href'] === '/new'
	&& $replacement->getImageMapAreas() === []);

$maxMap = array_fill(0, 4096, ['href' => '#x', 'tooltip' => 't']);
$maxChart = (new FastChart\BarChart(240, 180))
	->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
	->setSeries([['data' => [1]]])
	->setImageMap($maxMap);
unset($maxMap);
$maxChart->renderSvg();
$maxAreas = $maxChart->getImageMapAreas();
$maxChart->setImageMap([]);
result('max_entries_short_strings',
	count($maxAreas) === 1
	&& $maxAreas[0]['href'] === '#x'
	&& $maxAreas[0]['tooltip'] === 't'
	&& $maxChart->getImageMapAreas() === []);

$pointInput = [[1, 2, 'href' => '/scatter-old', 'tooltip' => 'old']];
$scatter = (new FastChart\ScatterChart(300, 220))
	->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
	->setPoints($pointInput);
$pointInput[0]['href'] = '/mutated';
$scatter->renderSvg();
$scatter->setPoints([[3, 4, 'href' => '/scatter-new', 'tooltip' => 'new']]);
$scatterReset = $scatter->getImageMapAreas() === [];
$scatter->renderSvg();
$scatterClone = clone $scatter;
unset($scatter, $pointInput);
gc_collect_cycles();
$scatterClone->renderSvg();
$scatterAreas = $scatterClone->getImageMapAreas();
result('scatter_replace_clone_outlives',
	$scatterReset
	&& count($scatterAreas) === 1
	&& $scatterAreas[0]['href'] === '/scatter-new'
	&& $scatterAreas[0]['tooltip'] === 'new');

$failedScatter = (new FastChart\ScatterChart(300, 220))
	->setPoints([[1, 1, 'href' => '/stale']]);
$failedScatter->renderSvg();
$failedScatter->setYAxisRange(100, null);
try {
	$failedScatter->renderSvg();
} catch (ValueError $e) {
}
result('failed_scatter_render_clears_artifact',
	$failedScatter->getImageMapAreas() === []);

$lifecycleOk = true;
for ($i = 0; $i < 40; $i++) {
	if (($i & 1) === 0) {
		$c = barWithMap([['href' => "/bar-$i"], ['href' => '#two']]);
	} else {
		$c = (new FastChart\ScatterChart(240, 180))
			->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
			->setPoints([[1, 1, 'href' => "/scatter-$i", 'tooltip' => 't']]);
	}
	$c->renderSvg();
	$c->renderSvg();
	if (count($c->getImageMapAreas()) < 1) {
		$lifecycleOk = false;
	}
	unset($c);
}
gc_collect_cycles();
result('repeated_lifecycle', $lifecycleOk);

?>
--EXPECT--
source_released: yes
repeated_reset_repopulate: yes
base_clone_outlives_source: yes
map_replacement_resets_artifact: yes
max_entries_short_strings: yes
scatter_replace_clone_outlives: yes
failed_scatter_render_clears_artifact: yes
repeated_lifecycle: yes
