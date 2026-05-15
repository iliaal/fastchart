--TEST--
Funnel::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
simplexml
--INI--
asan.detect_leaks=0
--FILE--
<?php
$c = new FastChart\Funnel(480, 320);
$c->setStages([
    ['label' => 'Visited',   'value' => 1000],
    ['label' => 'Signed up', 'value' =>  600],
    ['label' => 'Trial',     'value' =>  300],
    ['label' => 'Paid',      'value' =>  120],
])
  ->setTitle("Funnel SVG");

$c->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
$svg = $c->renderSvg();

var_dump(is_string($svg));
var_dump(strlen($svg) > 300);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(strpos($svg, "<svg ") !== false);
var_dump(str_ends_with(trim($svg), "</svg>"));

libxml_use_internal_errors(true);
$xml = simplexml_load_string($svg);
var_dump($xml instanceof SimpleXMLElement);

// 4 stages * (filled + outlined) trapezoid = at least 8 polygons.
$poly_n = substr_count($svg, "<polygon");
echo "polygon>=8: ", ($poly_n >= 8 ? "yes" : "no ($poly_n)"), "\n";

var_dump(strpos($svg, "Funnel SVG") !== false);
var_dump(strpos($svg, 'viewBox="0 0 480 320"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
polygon>=8: yes
bool(true)
bool(true)
OK
