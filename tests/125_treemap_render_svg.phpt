--TEST--
Treemap::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
$c = new FastChart\Treemap(480, 320);
$c->setItems([
    ['label' => 'Alpha',   'value' => 100],
    ['label' => 'Bravo',   'value' =>  70],
    ['label' => 'Charlie', 'value' =>  40],
    ['label' => 'Delta',   'value' =>  30, 'color' => 0xFF8800],
    ['label' => 'Echo',    'value' =>  15],
])
  ->setTitle('Treemap SVG');

$svg = $c->renderSvg();

var_dump(is_string($svg));
var_dump(strlen($svg) > 400);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(str_ends_with(trim($svg), "</svg>"));
libxml_use_internal_errors(true);
var_dump(simplexml_load_string($svg) instanceof SimpleXMLElement);

// 5 cells * (fill + border) + 1 bg = at least 11 rects.
$rect_n = substr_count($svg, "<rect");
echo "rect>=11: ", ($rect_n >= 11 ? "yes" : "no ($rect_n)"), "\n";

var_dump(strpos($svg, "Treemap SVG") !== false);
var_dump(strpos($svg, 'viewBox="0 0 480 320"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
rect>=11: yes
bool(true)
bool(true)
OK
