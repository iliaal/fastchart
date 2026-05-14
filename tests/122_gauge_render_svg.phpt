--TEST--
GaugeChart::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
$c = new FastChart\GaugeChart(400, 280);
$c->setRange(0, 100)
  ->setValue(72)
  ->setZones([
      ['from' => 0,  'to' => 50, 'color' => 0x4FB286],
      ['from' => 50, 'to' => 80, 'color' => 0xF4C842],
      ['from' => 80, 'to' => 100, 'color' => 0xE34A6F],
  ])
  ->setTitle('Gauge SVG');

$c->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
$svg = $c->renderSvg();

var_dump(is_string($svg));
var_dump(strlen($svg) > 400);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(str_ends_with(trim($svg), "</svg>"));
libxml_use_internal_errors(true);
var_dump(simplexml_load_string($svg) instanceof SimpleXMLElement);

// Background wedge + 3 zones = at least 4 <path> elements (arcs).
$path_n = substr_count($svg, "<path");
echo "path>=4: ", ($path_n >= 4 ? "yes" : "no ($path_n)"), "\n";

// Hole cutout + hub = at least 2 circles.
$circ_n = substr_count($svg, "<circle");
echo "circle>=2: ", ($circ_n >= 2 ? "yes" : "no ($circ_n)"), "\n";

// Needle line.
$line_n = substr_count($svg, "<line");
echo "line>=1: ", ($line_n >= 1 ? "yes" : "no ($line_n)"), "\n";

var_dump(strpos($svg, "Gauge SVG") !== false);
var_dump(strpos($svg, 'viewBox="0 0 400 280"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
path>=4: yes
circle>=2: yes
line>=1: yes
bool(true)
bool(true)
OK
