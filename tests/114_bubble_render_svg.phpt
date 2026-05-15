--TEST--
BubbleChart::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
simplexml
--INI--
asan.detect_leaks=0
--FILE--
<?php
$pts = [];
for ($i = 0; $i < 12; $i++) {
    $pts[] = [$i + cos($i) * 0.5, 10 + $i * 2 + sin($i) * 3, 5 + ($i % 4) * 3];
}

$c = new FastChart\BubbleChart(480, 320);
$c->setPoints($pts)
  ->setTitle("Bubble SVG");

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

// 12 bubbles * (fill + outline) = at least 24 circle elements
// (the SVG backend emits <circle> when rx == ry).
$ell_n = substr_count($svg, "<circle");
echo "circle>=24: ", ($ell_n >= 24 ? "yes" : "no ($ell_n)"), "\n";

var_dump(strpos($svg, "Bubble SVG") !== false);
var_dump(strpos($svg, 'viewBox="0 0 480 320"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
circle>=24: yes
bool(true)
bool(true)
OK
