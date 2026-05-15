--TEST--
LinearMeter::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
simplexml
--INI--
asan.detect_leaks=0
--FILE--
<?php
$c = new FastChart\LinearMeter(480, 160);
$c->setRange(0, 100)
  ->setValue(72)
  ->setZones([
      ['from' => 0,  'to' => 50, 'color' => 0x4FB286],
      ['from' => 50, 'to' => 80, 'color' => 0xF4C842],
      ['from' => 80, 'to' => 100, 'color' => 0xE34A6F],
  ])
  ->setTitle('LinearMeter SVG');

$c->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
$svg = $c->renderSvg();

var_dump(is_string($svg));
var_dump(strlen($svg) > 400);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(strpos($svg, "<svg ") !== false);
var_dump(str_ends_with(trim($svg), "</svg>"));

libxml_use_internal_errors(true);
$xml = simplexml_load_string($svg);
var_dump($xml instanceof SimpleXMLElement);

// Background fill + 3 zones + outline = several rects.
$rect_n = substr_count($svg, "<rect");
echo "rect>=5: ", ($rect_n >= 5 ? "yes" : "no ($rect_n)"), "\n";

// Pointer triangle = at least one polygon.
$poly_n = substr_count($svg, "<polygon");
echo "polygon>=1: ", ($poly_n >= 1 ? "yes" : "no ($poly_n)"), "\n";

var_dump(strpos($svg, "LinearMeter SVG") !== false);
var_dump(strpos($svg, 'viewBox="0 0 480 160"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
rect>=5: yes
polygon>=1: yes
bool(true)
bool(true)
OK
