--TEST--
NetworkChart caches deterministic layouts and invalidates every layout input
--EXTENSIONS--
fastchart
--FILE--
<?php

function network(array $nodes, array $links, int $seed = 17,
	int $iterations = 80, int $width = 420, int $height = 300): FastChart\NetworkChart {
	return (new FastChart\NetworkChart($width, $height))
		->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
		->setNodes($nodes)
		->setLinks($links)
		->setSeed($seed)
		->setIterations($iterations);
}

function median(array $values): float {
	sort($values);
	return $values[intdiv(count($values), 2)];
}

$nodes = [];
$links = [];
for ($i = 0; $i < 12; $i++) {
	$nodes[] = ['label' => "N$i", 'color' => 0x336699 + $i];
	if ($i > 0) {
		$links[] = ['from' => $i - 1, 'to' => $i, 'value' => $i + 1];
	}
}

$chart = network($nodes, $links);
$first = $chart->renderSvg();
echo 'repeat_identity: ', $chart->renderSvg() === $first ? "yes\n" : "NO\n";

$chart = network($nodes, $links);
$chart->renderSvg();
$chart->setSeed(99);
echo 'seed_invalidates: ',
	$chart->renderSvg() === network($nodes, $links, 99)->renderSvg()
		? "yes\n" : "NO\n";

$chart = network($nodes, $links);
$chart->renderSvg();
$chart->setIterations(31);
echo 'iterations_invalidates: ',
	$chart->renderSvg() === network($nodes, $links, 17, 31)->renderSvg()
		? "yes\n" : "NO\n";

$alternateLinks = [
	['from' => 0, 'to' => 6, 'value' => 2],
	['from' => 1, 'to' => 9, 'value' => 3],
];
$chart = network($nodes, $links);
$chart->renderSvg();
$chart->setLinks($alternateLinks);
echo 'links_invalidate: ',
	$chart->renderSvg() === network($nodes, $alternateLinks)->renderSvg()
		? "yes\n" : "NO\n";

$fewerNodes = array_slice($nodes, 0, 7);
$chart = network($nodes, $links);
$chart->renderSvg();
$chart->setNodes($fewerNodes);
echo 'nodes_invalidate: ',
	$chart->renderSvg() === network($fewerNodes, [])->renderSvg()
		? "yes\n" : "NO\n";

$chart = network($nodes, $links);
$chart->renderSvg();
$chart->setSize(500, 340);
echo 'size_rekeys: ',
	$chart->renderSvg() === network($nodes, $links, 17, 80, 500, 340)->renderSvg()
		? "yes\n" : "NO\n";

$chart = network($nodes, $links);
$chart->renderSvg();
$chart->setPlotRect(25, 30, 360, 250);
$fresh = network($nodes, $links)->setPlotRect(25, 30, 360, 250);
echo 'plot_rect_rekeys: ', $chart->renderSvg() === $fresh->renderSvg()
	? "yes\n" : "NO\n";

$chart = network($nodes, $links);
$chart->renderSvg();
$chart->setTitle('Changed layout');
$fresh = network($nodes, $links)->setTitle('Changed layout');
echo 'title_rekeys: ', $chart->renderSvg() === $fresh->renderSvg()
	? "yes\n" : "NO\n";

$source = network($nodes, $links);
$source->renderSvg();
$clone = clone $source;
unset($source);
gc_collect_cycles();
echo 'clone_outlives_source: ',
	$clone->renderSvg() === network($nodes, $links)->renderSvg()
		? "yes\n" : "NO\n";

$chart = network($nodes, $links);
$svg = $chart->renderSvg();
$png = $chart->renderPng();
$jpeg = $chart->renderJpeg();
$webp = $chart->renderWebp();
$pdfOk = true;
try {
	$pdfOk = str_starts_with($chart->renderPdf(), '%PDF');
} catch (Error $e) {
	$pdfOk = str_contains($e->getMessage(), 'PDF support not compiled in');
}
$formats = str_starts_with($png, "\x89PNG")
	&& str_starts_with($jpeg, "\xFF\xD8")
	&& str_starts_with($webp, 'RIFF')
	&& $pdfOk
	&& $chart->renderSvg() === $svg;
echo 'mixed_formats: ', $formats ? "yes\n" : "NO\n";

$perfNodes = [];
$perfLinks = [];
for ($i = 0; $i < 64; $i++) {
	$perfNodes[] = ['label' => "P$i"];
	if ($i > 0) {
		$perfLinks[] = ['from' => $i - 1, 'to' => $i, 'value' => 1];
	}
	if ($i >= 8) {
		$perfLinks[] = ['from' => $i - 8, 'to' => $i, 'value' => 1];
	}
}
$perf = network($perfNodes, $perfLinks, 17, 1000, 640, 480);
$start = hrtime(true);
$perf->renderSvg();
$cold = hrtime(true) - $start;
$hotSamples = [];
for ($i = 0; $i < 5; $i++) {
	$start = hrtime(true);
	$perf->renderSvg();
	$hotSamples[] = hrtime(true) - $start;
}
echo 'hot_gain_80pct: ', median($hotSamples) < $cold * 0.2 ? "yes\n" : "NO\n";

?>
--EXPECT--
repeat_identity: yes
seed_invalidates: yes
iterations_invalidates: yes
links_invalidate: yes
nodes_invalidate: yes
size_rekeys: yes
plot_rect_rekeys: yes
title_rekeys: yes
clone_outlives_source: yes
mixed_formats: yes
hot_gain_80pct: yes
