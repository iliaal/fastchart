--TEST--
Stock volatility is invariant under a large constant price offset
--EXTENSIONS--
fastchart
--FILE--
<?php

function rows(float $base): array {
	$out = [];
	for ($i = 0; $i < 30; $i++) {
		$close = $base + ($i % 2);
		$out[] = [1700000000 + $i, $close, $close + 2,
			$close - 2, $close, 1000];
	}
	return $out;
}

function pane_ys(string $svg): array {
	preg_match_all(
		'/<line x1="[-0-9.]+" y1="([-0-9.]+)" x2="[-0-9.]+" y2="([-0-9.]+)"[^>]*stroke-width="2"/',
		$svg, $matches, PREG_SET_ORDER
	);
	$ys = [];
	foreach ($matches as $line) {
		$ys[$line[1]] = true;
		$ys[$line[2]] = true;
	}
	$ys = array_map('floatval', array_keys($ys));
	sort($ys);
	return $ys;
}

function bollinger_lines(string $svg): array {
	preg_match_all('/<line[^>]*stroke="#123456"[^>]*>/', $svg, $matches);
	return $matches[0];
}

$base = (new FastChart\StockChart(600, 400))
	->setOhlcv(rows(0))
	->addStdDev(5)
	->renderSvg();
$shifted = (new FastChart\StockChart(600, 400))
	->setOhlcv(rows(1.0e12))
	->addStdDev(5)
	->renderSvg();

echo 'stddev_base_flat: ', count(pane_ys($base)) === 1 ? "yes\n" : "NO\n";
echo 'stddev_shifted_flat: ', count(pane_ys($shifted)) === 1 ? "yes\n" : "NO\n";
echo 'stddev_same_geometry: ', pane_ys($base) === pane_ys($shifted) ? "yes\n" : "NO\n";

$colors = [0x111111, 0x123456, 0x222222];
$bollBase = (new FastChart\StockChart(600, 400))
	->setSeriesColors($colors)
	->setOhlcv(rows(0))
	->addBollingerBands(5, 2.0)
	->renderSvg();
$bollShifted = (new FastChart\StockChart(600, 400))
	->setSeriesColors($colors)
	->setOhlcv(rows(1.0e12))
	->addBollingerBands(5, 2.0)
	->renderSvg();

$a = bollinger_lines($bollBase);
$b = bollinger_lines($bollShifted);
echo 'bollinger_present: ', count($a) > 20 ? "yes\n" : "NO\n";
echo 'bollinger_translation_invariant: ', $a === $b ? "yes\n" : "NO\n";

$constant = [];
for ($i = 0; $i < 30; $i++) {
	$constant[] = [1700000000 + $i, 1.0e308, 1.0e308,
		1.0e308, 1.0e308, 1000];
}
foreach ([2, 29] as $period) {
	$svg = (new FastChart\StockChart(600, 400))
		->setOhlcv($constant)
		->addStdDev($period)
		->renderSvg();
	echo "constant_period_$period: ",
		count(pane_ys($svg)) === 1 ? "yes\n" : "NO\n";
}

function extreme_rows(float $divisor): array {
	$values = [1.7e308, -1.7e308, 0.0, 1.0e308, -1.0e308, 0.0];
	$out = [];
	foreach ($values as $i => $value) {
		$close = $value / $divisor;
		$out[] = [1700001000 + $i, $close, $close, $close, $close, 1000];
	}
	return $out;
}

$extreme = (new FastChart\StockChart(600, 400))
	->setOhlcv(extreme_rows(1.0))
	->addStdDev(2)
	->renderSvg();
$scaled = (new FastChart\StockChart(600, 400))
	->setOhlcv(extreme_rows(1.0e158))
	->addStdDev(2)
	->renderSvg();
echo 'opposite_extremes_nonflat: ',
	count(pane_ys($extreme)) > 1 && count(pane_ys($scaled)) > 1
		? "yes\n" : "NO\n";

$cached = (new FastChart\StockChart(600, 400))
	->setOhlcv(rows(1.0e12))
	->addBollingerBands(5, 2.0);
$cachedClone = clone $cached;
$cachedSvg = $cached->addStdDev(5)->renderSvg();
$cloneSvg = $cachedClone->addStdDev(5)->renderSvg();
$uncachedSvg = (new FastChart\StockChart(600, 400))
	->setOhlcv(rows(1.0e12))
	->addStdDev(5)
	->addBollingerBands(5, 2.0)
	->renderSvg();
echo 'cached_matches_uncached: ', $cachedSvg === $uncachedSvg ? "yes\n" : "NO\n";
echo 'cached_clone_independent: ', $cloneSvg === $cachedSvg ? "yes\n" : "NO\n";

$replacement = [];
for ($i = 0; $i < 30; $i++) {
	$close = ($i % 3) * 10.0;
	$replacement[] = [1700010000 + $i, $close, $close + 2,
		$close - 2, $close, 1000];
}
$reusedSvg = $cached->setOhlcv($replacement)->addStdDev(5)->renderSvg();
$freshSvg = (new FastChart\StockChart(600, 400))
	->setOhlcv($replacement)
	->addStdDev(5)
	->renderSvg();
echo 'cache_invalidated_on_data_change: ',
	$reusedSvg === $freshSvg ? "yes\n" : "NO\n";

?>
--EXPECT--
stddev_base_flat: yes
stddev_shifted_flat: yes
stddev_same_geometry: yes
bollinger_present: yes
bollinger_translation_invariant: yes
constant_period_2: yes
constant_period_29: yes
opposite_extremes_nonflat: yes
cached_matches_uncached: yes
cached_clone_independent: yes
cache_invalidated_on_data_change: yes
