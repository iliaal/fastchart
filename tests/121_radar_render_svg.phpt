--TEST--
RadarChart::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
$c = new FastChart\RadarChart(420, 420);
$c->setSeries([
    ['label' => 'team A', 'data' => [80, 70, 60, 85, 75]],
    ['label' => 'team B', 'data' => [60, 80, 75, 50, 90]],
])
  ->setCategoryLabels(['Speed', 'Power', 'Endur', 'Skill', 'Mind'])
  ->setFilled(true)
  ->setTitle('Radar SVG');

$c->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
$svg = $c->renderSvg();

var_dump(is_string($svg));
var_dump(strlen($svg) > 500);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(str_ends_with(trim($svg), "</svg>"));
libxml_use_internal_errors(true);
var_dump(simplexml_load_string($svg) instanceof SimpleXMLElement);

// 4 grid rings + 2 series polygons + (filled overlay for 2 series) = at least 8 polygons.
$poly_n = substr_count($svg, "<polygon");
echo "polygon>=8: ", ($poly_n >= 8 ? "yes" : "no ($poly_n)"), "\n";

// 5 spokes.
$line_n = substr_count($svg, "<line");
echo "line>=5: ", ($line_n >= 5 ? "yes" : "no ($line_n)"), "\n";

var_dump(strpos($svg, "Radar SVG") !== false);
var_dump(strpos($svg, "Speed") !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
polygon>=8: yes
line>=5: yes
bool(true)
bool(true)
OK
