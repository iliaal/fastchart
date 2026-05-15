--TEST--
ContourChart::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
simplexml
--INI--
asan.detect_leaks=0
--FILE--
<?php
$rows = 12;
$cols = 16;
$grid = [];
for ($i = 0; $i < $rows; $i++) {
    $row = [];
    for ($j = 0; $j < $cols; $j++) {
        $row[] = sin($i / 2.0) * cos($j / 2.0) * 10 + ($i + $j) * 0.4;
    }
    $grid[] = $row;
}

$c = new FastChart\ContourChart(480, 320);
$c->setGrid($grid)
  ->setLevels([-5, 0, 5, 8])
  ->setFilled(true)
  ->setColorRamp(0x2E5CB8, 0xE34A6F)
  ->setTitle('Contour SVG');

$c->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
$svg = $c->renderSvg();

var_dump(is_string($svg));
var_dump(strlen($svg) > 500);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(str_ends_with(trim($svg), "</svg>"));
libxml_use_internal_errors(true);
var_dump(simplexml_load_string($svg) instanceof SimpleXMLElement);

// Filled mode: a rect per cell (11*15 = 165), plus background + frame.
$rect_n = substr_count($svg, "<rect");
echo "rect>=150: ", ($rect_n >= 150 ? "yes" : "no ($rect_n)"), "\n";

// Isolines emit many <line> segments.
$line_n = substr_count($svg, "<line");
echo "line>=10: ", ($line_n >= 10 ? "yes" : "no ($line_n)"), "\n";

var_dump(strpos($svg, "Contour SVG") !== false);
var_dump(strpos($svg, 'viewBox="0 0 480 320"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
rect>=150: yes
line>=10: yes
bool(true)
bool(true)
OK
