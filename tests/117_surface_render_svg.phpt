--TEST--
SurfaceChart::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
$grid = [];
for ($r = 0; $r < 4; $r++) {
    $row = [];
    for ($cc = 0; $cc < 6; $cc++) $row[] = $r * 6 + $cc + 1;
    $grid[] = $row;
}

$c = new FastChart\SurfaceChart(480, 320);
$c->setGrid($grid)
  ->setColorRamp(0x0000FF, 0xFF0000)
  ->setTitle('Surface SVG');

$svg = $c->renderSvg();

var_dump(is_string($svg));
var_dump(strlen($svg) > 400);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(strpos($svg, "<svg ") !== false);
var_dump(str_ends_with(trim($svg), "</svg>"));

libxml_use_internal_errors(true);
$xml = simplexml_load_string($svg);
var_dump($xml instanceof SimpleXMLElement);

// 4x6 = 24 cell rects + background + title backing.
$rect_n = substr_count($svg, "<rect");
echo "rect>=24: ", ($rect_n >= 24 ? "yes" : "no ($rect_n)"), "\n";

var_dump(strpos($svg, "Surface SVG") !== false);
var_dump(strpos($svg, 'viewBox="0 0 480 320"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
rect>=24: yes
bool(true)
bool(true)
OK
