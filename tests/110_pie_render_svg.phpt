--TEST--
PieChart::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
$c = new FastChart\PieChart(360, 320);
$c->setSlices(["alpha" => 25, "beta" => 40, "gamma" => 15, "delta" => 20])
  ->setTitle("Pie SVG");

$svg = $c->renderSvg();

// Basic shape.
var_dump(is_string($svg));
var_dump(strlen($svg) > 500);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(str_ends_with(trim($svg), "</svg>"));

// Round-trip parse.
libxml_use_internal_errors(true);
$xml = simplexml_load_string($svg);
var_dump($xml instanceof SimpleXMLElement);

// One filled <path> + one stroked <path> per slice = 8 paths for 4 slices.
$path_n = substr_count($svg, "<path");
echo "paths>=8: ", ($path_n >= 8 ? "yes" : "no ($path_n)"), "\n";

// Title text present.
var_dump(strpos($svg, "Pie SVG") !== false);

// Viewport.
var_dump(strpos($svg, 'viewBox="0 0 360 320"') !== false);

// Donut variant: a center disk is emitted as a circle (rx == ry).
$d = new FastChart\PieChart(300, 300);
$d->setSlices(["a" => 30, "b" => 25, "c" => 20, "d" => 25])
  ->setDonutHoleRatio(0.4);
$svg2 = $d->renderSvg();
$circle_n = substr_count($svg2, "<circle");
echo "donut hole circle>=1: ", ($circle_n >= 1 ? "yes" : "no ($circle_n)"), "\n";
var_dump(simplexml_load_string($svg2) instanceof SimpleXMLElement);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
paths>=8: yes
bool(true)
bool(true)
donut hole circle>=1: yes
bool(true)
OK
