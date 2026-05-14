--TEST--
GanttChart::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
$c = new FastChart\GanttChart(640, 320);
$c->setTitle('Gantt SVG')
  ->setTasks([
      ['name' => 'Plan',  'start' => 1700000000, 'end' => 1700432000],
      ['name' => 'Build', 'start' => 1700432000, 'end' => 1701036800, 'depends' => [0]],
      ['name' => 'Test',  'start' => 1700864000, 'end' => 1701123200, 'depends' => [1], 'color' => 0xFF8800],
      ['name' => 'Ship',  'start' => 1701123200, 'end' => 1701123200, 'milestone' => true, 'depends' => [2]],
  ]);

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

// 3 bar rects + 1 milestone polygon.
$rect_n = substr_count($svg, "<rect");
echo "rect>=3: ", ($rect_n >= 3 ? "yes" : "no ($rect_n)"), "\n";
$poly_n = substr_count($svg, "<polygon");
echo "polygon>=1: ", ($poly_n >= 1 ? "yes" : "no ($poly_n)"), "\n";

var_dump(strpos($svg, "Gantt SVG") !== false);
var_dump(strpos($svg, 'viewBox="0 0 640 320"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
rect>=3: yes
polygon>=1: yes
bool(true)
bool(true)
OK
