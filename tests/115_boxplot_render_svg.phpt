--TEST--
BoxPlot::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
$c = new FastChart\BoxPlot(480, 320);
$c->setBoxes([
    [10, 25, 35, 45, 60],
    ['min' => 5,  'q1' => 20, 'median' => 30,
     'q3' => 40, 'max' => 55, 'outliers' => [2, 70], 'label' => 'B'],
    [15, 28, 38, 48, 65],
])
  ->setCategoryLabels(['A', 'B', 'C'])
  ->setTitle("BoxPlot SVG");

$svg = $c->renderSvg();

var_dump(is_string($svg));
var_dump(strlen($svg) > 400);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(strpos($svg, "<svg ") !== false);
var_dump(str_ends_with(trim($svg), "</svg>"));

libxml_use_internal_errors(true);
$xml = simplexml_load_string($svg);
var_dump($xml instanceof SimpleXMLElement);

// 3 boxes: each has filled rect + outlined rect = at least 6 rects.
$rect_n = substr_count($svg, "<rect");
echo "rect>=6: ", ($rect_n >= 6 ? "yes" : "no ($rect_n)"), "\n";

// 4 whisker lines + median per box = many lines.
$line_n = substr_count($svg, "<line");
echo "line>=12: ", ($line_n >= 12 ? "yes" : "no ($line_n)"), "\n";

// 2 outliers as circles.
$circ_n = substr_count($svg, "<circle");
echo "circle>=2: ", ($circ_n >= 2 ? "yes" : "no ($circ_n)"), "\n";

var_dump(strpos($svg, "BoxPlot SVG") !== false);
var_dump(strpos($svg, 'viewBox="0 0 480 320"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
rect>=6: yes
line>=12: yes
circle>=2: yes
bool(true)
bool(true)
OK
