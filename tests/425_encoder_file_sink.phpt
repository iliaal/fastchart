--TEST--
Raster renderToFile streams exact bytes atomically for charts and symbols
--EXTENSIONS--
fastchart
--FILE--
<?php

function result(string $label, bool $ok): void {
	echo $label, ': ', $ok ? "yes\n" : "NO\n";
}

$dir = sys_get_temp_dir() . '/fastchart-sink-' . bin2hex(random_bytes(6));
mkdir($dir, 0700);

$chart = (new FastChart\LineChart(360, 220))
	->setSeries([1, 4, 2, 5]);
$formats = [
	['png', fn() => $chart->renderPng()],
	['jpg', fn() => $chart->renderJpeg(83)],
	['webp', fn() => $chart->renderWebp(83)],
];
foreach ($formats as [$extension, $render]) {
	$expected = $render();
	$path = "$dir/chart.$extension";
	$written = $chart->renderToFile($path, 83);
	result("chart_$extension",
		$written === strlen($expected)
		&& file_get_contents($path) === $expected);
}

$symbol = (new FastChart\Code128())->setData('STREAM-425');
$symbolFormats = [
	['png', fn() => $symbol->renderPng()],
	['jpg', fn() => $symbol->renderJpeg(81)],
	['webp', fn() => $symbol->renderWebp(81)],
];
foreach ($symbolFormats as [$extension, $render]) {
	$expected = $render();
	$path = "$dir/symbol.$extension";
	$written = $symbol->renderToFile($path, 81);
	result("symbol_$extension",
		$written === strlen($expected)
		&& file_get_contents($path) === $expected);
}

$preserved = "$dir/preserved.png";
file_put_contents($preserved, 'old-good-data');
$broken = (new FastChart\LineChart(320, 200))
	->setSeries([1, 2, 3])
	->setYAxisRange(100, null);
try {
	$broken->renderToFile($preserved);
} catch (ValueError $e) {
}
result('chart_failure_atomic',
	file_get_contents($preserved) === 'old-good-data'
	&& glob($preserved . '.fctmp-*') === []);

$missing = "$dir/missing.png";
try {
	(new FastChart\Code128())->renderToFile($missing);
} catch (Error $e) {
}
result('symbol_failure_cleanup',
	!file_exists($missing)
	&& glob($missing . '.fctmp-*') === []);

foreach (glob($dir . '/*') as $path) {
	unlink($path);
}
rmdir($dir);

?>
--EXPECT--
chart_png: yes
chart_jpg: yes
chart_webp: yes
symbol_png: yes
symbol_jpg: yes
symbol_webp: yes
chart_failure_atomic: yes
symbol_failure_cleanup: yes
