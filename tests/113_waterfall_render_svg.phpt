--TEST--
Waterfall::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
simplexml
--INI--
asan.detect_leaks=0
--FILE--
<?php
$c = new FastChart\Waterfall(480, 320);
$c->setBars([
    ['label' => 'Start', 'value' => 100, 'kind' => 'total'],
    ['label' => 'Add',   'value' =>  30],
    ['label' => 'Drop',  'value' => -40],
    ['label' => 'Add2',  'value' =>  20],
    ['label' => 'End',   'value' => 110, 'kind' => 'total'],
])
  ->setTitle("Waterfall SVG");

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

// 5 bars: each has fill rect + outline rect = at least 10 rects.
$rect_n = substr_count($svg, "<rect");
echo "rect>=10: ", ($rect_n >= 10 ? "yes" : "no ($rect_n)"), "\n";

var_dump(strpos($svg, "Waterfall SVG") !== false);
var_dump(strpos($svg, 'viewBox="0 0 480 320"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
rect>=10: yes
bool(true)
bool(true)
OK
