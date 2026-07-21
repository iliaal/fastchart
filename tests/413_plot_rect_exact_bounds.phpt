--TEST--
setPlotRect uses exact inclusive bounds clamped to the current canvas
--EXTENSIONS--
fastchart
--FILE--
<?php

function plot_rect(string $svg): string {
	preg_match('/<rect x="(\d+)" y="(\d+)" width="(\d+)" height="(\d+)" fill="#[0-9A-F]+"\/>/',
		$svg, $match);
	return isset($match[1]) ? implode(',', array_slice($match, 1, 4)) : 'missing';
}

function line_chart(int $width, int $height): FastChart\LineChart {
	return (new FastChart\LineChart($width, $height))->setSeries([1, 2]);
}

echo 'tiny: ', plot_rect(line_chart(20, 20)->setPlotRect(1, 1, 5, 5)->renderSvg()), "\n";
echo 'one_pixel: ', plot_rect(line_chart(20, 20)->setPlotRect(7, 8, 7, 8)->renderSvg()), "\n";
echo 'edge_clamped: ', plot_rect(line_chart(20, 20)->setPlotRect(15, 15, 30, 30)->renderSvg()), "\n";
echo 'outside_clamped: ', plot_rect(line_chart(20, 20)->setPlotRect(30, 30, 40, 40)->renderSvg()), "\n";

$resized = line_chart(200, 200)->setPlotRect(10, 10, 100, 100);
$resized->setSize(20, 20);
echo 'smaller: ', plot_rect($resized->renderSvg()), "\n";
$resized->setSize(200, 200);
echo 'restored: ', plot_rect($resized->renderSvg()), "\n";

$heatmap = (new FastChart\Heatmap(20, 20))
	->setGrid([[1, 2], [3, 4]])
	->setPlotRect(15, 15, 30, 30)
	->renderSvg();
preg_match_all('/<rect x="(\d+)" y="(\d+)" width="(\d+)" height="(\d+)"/',
	$heatmap, $heatRects, PREG_SET_ORDER);
$heatBounded = count($heatRects) >= 4;
foreach ($heatRects as $rect) {
	$heatBounded = $heatBounded
		&& (int)$rect[1] + (int)$rect[3] <= 20
		&& (int)$rect[2] + (int)$rect[4] <= 20;
}
echo 'heatmap_clamped: ', $heatBounded ? "yes\n" : "NO\n";

$treemap = (new FastChart\Treemap(20, 20))
	->setItems([['value' => 1, 'color' => 0x123456]])
	->setPlotRect(15, 15, 30, 30)
	->renderSvg();
echo 'treemap_clamped: ',
	str_contains($treemap, '<rect x="15" y="15" width="5" height="5"')
		? "yes\n" : "NO\n";

$timeline = (new FastChart\SerpentineTimeline(20, 20))
	->setEvents([['label' => 'A'], ['label' => 'B']])
	->setPlotRect(5, 5, 30, 30)
	->renderSvg();
preg_match_all('/\b(?:x1|x2|cx)="(-?\d+)/', $timeline, $coords);
$bounded = !$coords[1] || max(array_map('intval', $coords[1])) <= 19;
echo 'timeline_clamped: ', $bounded ? "yes\n" : "NO\n";

?>
--EXPECT--
tiny: 1,1,5,5
one_pixel: 7,8,1,1
edge_clamped: 15,15,5,5
outside_clamped: 19,19,1,1
smaller: 10,10,10,10
restored: 10,10,91,91
heatmap_clamped: yes
treemap_clamped: yes
timeline_clamped: yes
