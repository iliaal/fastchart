--TEST--
Heatmap::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
$grid = [];
for ($r = 0; $r < 5; $r++) {
    $row = [];
    for ($cc = 0; $cc < 7; $cc++) $row[] = $r * 7 + $cc + 1;
    $grid[] = $row;
}

$c = new FastChart\Heatmap(560, 320);
$c->setGrid($grid)
  ->setTitle('Heatmap SVG');

$c->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
$svg = $c->renderSvg();

var_dump(is_string($svg));
var_dump(strlen($svg) > 500);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(strpos($svg, "<svg ") !== false);
var_dump(str_ends_with(trim($svg), "</svg>"));

libxml_use_internal_errors(true);
$xml = simplexml_load_string($svg);
var_dump($xml instanceof SimpleXMLElement);

// 5x7 = 35 cells * 2 rects (fill + border) = at least 70.
$rect_n = substr_count($svg, "<rect");
echo "rect>=70: ", ($rect_n >= 70 ? "yes" : "no ($rect_n)"), "\n";

var_dump(strpos($svg, "Heatmap SVG") !== false);
var_dump(strpos($svg, 'viewBox="0 0 560 320"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
rect>=70: yes
bool(true)
bool(true)
OK
