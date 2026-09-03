--TEST--
StockChart::setOhlcv() leaves no stale image-map behind (mirrors 183)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Mirrors 183_scatter_setpoints_imagemap_stale.phpt on the StockChart
 * lane. StockChart renders no hot-spots, so the map is empty — the
 * contract is that swapping the candle buffer can neither resurrect
 * a stale map view nor crash: empty before, empty after the swap,
 * empty after a fresh render. */

$ohlcv = [];
for ($i = 0; $i < 8; $i++) {
	$ohlcv[] = [1700000000 + $i * 86400,
		100 + $i, 102 + $i, 99 + $i, 101 + $i, 1000];
}

$s = (new FastChart\StockChart(500, 300))->setOhlcv($ohlcv);
$s->setImageMap([['href' => '/first-target', 'tooltip' => 'first']]);
$s->renderSvg();

var_dump($s->getImageMap());
var_dump($s->getImageMapAreas());

/* Replace the data: entries must not resurrect a stale map. */
$s->setOhlcv(array_slice($ohlcv, 0, 4));
var_dump($s->getImageMap());

/* Fresh render regenerates from current state (still no hot-spots). */
$s->renderSvg();
var_dump($s->getImageMap());
var_dump($s->getImageMapAreas());

?>
--EXPECT--
string(0) ""
array(0) {
}
string(0) ""
string(0) ""
array(0) {
}
