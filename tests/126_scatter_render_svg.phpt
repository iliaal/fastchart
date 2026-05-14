--TEST--
ScatterChart::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
$pts = [];
for ($i = 0; $i < 24; $i++) {
    $pts[] = [$i + cos($i) * 0.3, 10 + $i * 1.5 + sin($i) * 2.5];
}

$c = new FastChart\ScatterChart(640, 360);
$c->setPoints($pts)
  ->setTrendLine(true)
  ->setTitle('Scatter SVG');

$svg = $c->renderSvg();

var_dump(is_string($svg));
var_dump(strlen($svg) > 500);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(str_ends_with(trim($svg), "</svg>"));
libxml_use_internal_errors(true);
var_dump(simplexml_load_string($svg) instanceof SimpleXMLElement);

// 24 markers default to circles.
$circ_n = substr_count($svg, "<circle");
echo "circle>=24: ", ($circ_n >= 24 ? "yes" : "no ($circ_n)"), "\n";

// Trend line: 200 sub-segments.
$line_n = substr_count($svg, "<line");
echo "line>=100: ", ($line_n >= 100 ? "yes" : "no ($line_n)"), "\n";

var_dump(strpos($svg, "Scatter SVG") !== false);
var_dump(strpos($svg, 'viewBox="0 0 640 360"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
circle>=24: yes
line>=100: yes
bool(true)
bool(true)
OK
