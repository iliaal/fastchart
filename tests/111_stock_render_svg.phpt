--TEST--
StockChart::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
/* 20-bar OHLCV with an SMA overlay and a volume pane. */
$rows = [];
$base = 100.0;
for ($i = 0; $i < 20; $i++) {
    $open  = $base + sin($i / 2.0) * 5;
    $close = $open + cos($i) * 3;
    $high  = max($open, $close) + 2;
    $low   = min($open, $close) - 2;
    $rows[] = [1700000000 + $i * 86400, $open, $high, $low, $close, 1000 + $i * 50];
    $base = $close;
}

$c = new FastChart\StockChart(640, 400);
$c->setOhlcv($rows)
  ->setTitle("Stock SVG")
  ->addMovingAverage(5, FastChart\StockChart::MA_SMA)
  ->setVolumePane(true);

$svg = $c->renderSvg();

// Basic shape.
var_dump(is_string($svg));
var_dump(strlen($svg) > 800);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(str_ends_with(trim($svg), "</svg>"));

// Round-trip parse.
libxml_use_internal_errors(true);
$xml = simplexml_load_string($svg);
var_dump($xml instanceof SimpleXMLElement);

// 20 candle bodies + 20 volume bars + frame/plot bg + pane borders.
$rect_n = substr_count($svg, "<rect");
echo "rect>=20: ", ($rect_n >= 20 ? "yes" : "no ($rect_n)"), "\n";

// The 5-bar SMA emits 15 segments + wick lines for each candle. Total
// <line> count is large.
$line_n = substr_count($svg, "<line");
echo "line>=20: ", ($line_n >= 20 ? "yes" : "no ($line_n)"), "\n";

// Title.
var_dump(strpos($svg, "Stock SVG") !== false);

// Viewport.
var_dump(strpos($svg, 'viewBox="0 0 640 400"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
rect>=20: yes
line>=20: yes
bool(true)
bool(true)
OK
