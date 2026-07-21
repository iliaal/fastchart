--TEST--
Large-window extrema indicators preserve values and newest-tie semantics
--EXTENSIONS--
fastchart
--FILE--
<?php

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

$rows = [];
for ($i = 0; $i < 70; $i++) {
	$close = $i < 63 ? 50 : min(100, 25 * ($i - 62));
	$rows[] = [1700000000 + $i, $close, 100, 0, $close, 1000];
}
$make = fn() => (new FastChart\StockChart(600, 400))->setOhlcv($rows);

$stoch = pane_ys($make()->addStochastic(64, 1)->renderSvg());
$williams = pane_ys($make()->addWilliamsR(64)->renderSvg());
echo 'stochastic_varies: ', count($stoch) >= 4 ? "yes\n" : "NO\n";
echo 'williams_matches_scale: ', $stoch === $williams ? "yes\n" : "NO\n";

$ties = [];
for ($i = 0; $i < 70; $i++) {
	$ties[] = [1700000000 + $i, 50, 100, 0, 50, 1000];
}
$aroon = pane_ys(
	(new FastChart\StockChart(600, 400))->setOhlcv($ties)->addAroon(64)->renderSvg()
);
echo 'aroon_newest_ties_stay_at_100: ', count($aroon) === 1 ? "yes\n" : "NO\n";

?>
--EXPECT--
stochastic_varies: yes
williams_matches_scale: yes
aroon_newest_ties_stay_at_100: yes
