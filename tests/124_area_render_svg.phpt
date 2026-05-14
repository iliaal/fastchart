--TEST--
AreaChart::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
$c = new FastChart\AreaChart(640, 320);
$c->setSeries([
    ['label' => 'A', 'data' => [10, 14, 12, 18, 22, 19, 25, 23]],
    ['label' => 'B', 'data' => [ 8, 11, 15, 13, 17, 21, 20, 24]],
])
  ->setCategoryLabels(["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug"])
  ->setTitle("Area SVG");

$svg = $c->renderSvg();

var_dump(is_string($svg));
var_dump(strlen($svg) > 500);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(str_ends_with(trim($svg), "</svg>"));
libxml_use_internal_errors(true);
var_dump(simplexml_load_string($svg) instanceof SimpleXMLElement);

// 2 series with translucent filled polygons.
$poly_n = substr_count($svg, "<polygon");
echo "polygon>=2: ", ($poly_n >= 2 ? "yes" : "no ($poly_n)"), "\n";

// Top-edge stroke segments (7 per series = 14 lines minimum).
$line_n = substr_count($svg, "<line");
echo "line>=10: ", ($line_n >= 10 ? "yes" : "no ($line_n)"), "\n";

var_dump(strpos($svg, "Area SVG") !== false);
var_dump(strpos($svg, 'viewBox="0 0 640 320"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
polygon>=2: yes
line>=10: yes
bool(true)
bool(true)
OK
