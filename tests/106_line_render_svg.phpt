--TEST--
LineChart::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
$c = new FastChart\LineChart(640, 320);
$c->setSeries([
    ['label' => 'alpha', 'data' => [10, 14, 12, 18, 22, 19, 25, 23]],
    ['label' => 'beta',  'data' => [ 8, 11, 15, 13, 17, 21, 20, 24]],
])
  ->setTitle("Quarterly revenue")
  ->setCategoryLabels(["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug"]);

$c->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
$svg = $c->renderSvg();

// Basic shape: non-empty, XML prolog + <svg> root.
var_dump(is_string($svg));
var_dump(strlen($svg) > 500);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(strpos($svg, "<svg ") !== false);
var_dump(str_ends_with(trim($svg), "</svg>"));

// Round-trip parse: must be valid XML.
libxml_use_internal_errors(true);
$xml = simplexml_load_string($svg);
var_dump($xml instanceof SimpleXMLElement);

// Element counts. A 2-series, 8-point line chart with title +
// category labels + axis ticks should produce a sizable number of
// <text> nodes (title, axis labels, tick labels) and circles for
// the markers (default marker = circle, size 6).
$text_n   = substr_count($svg, "<text");
$line_n   = substr_count($svg, "<line");
$rect_n   = substr_count($svg, "<rect");
$circle_n = substr_count($svg, "<circle");
echo "text>=4: ", ($text_n >= 4 ? "yes" : "no ($text_n)"), "\n";
echo "line>=4: ", ($line_n >= 4 ? "yes" : "no ($line_n)"), "\n";
echo "rect>=2: ", ($rect_n >= 2 ? "yes" : "no ($rect_n)"), "\n";
echo "circle>=8: ", ($circle_n >= 8 ? "yes" : "no ($circle_n)"), "\n";

// Title text must appear in the output (XML-escaped, but the
// chosen title has no special characters).
var_dump(strpos($svg, "Quarterly revenue") !== false);

// Viewport: width / height / viewBox must reflect setSize().
var_dump(strpos($svg, 'width="640"') !== false);
var_dump(strpos($svg, 'height="320"') !== false);
var_dump(strpos($svg, 'viewBox="0 0 640 320"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
text>=4: yes
line>=4: yes
rect>=2: yes
circle>=8: yes
bool(true)
bool(true)
bool(true)
bool(true)
OK
