--TEST--
BarChart::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
$c = new FastChart\BarChart(480, 280);
$c->setSeries([
    ['label' => 'alpha', 'data' => [10, 20, 30]],
    ['label' => 'beta',  'data' => [15, 12, 28]],
])
  ->setTitle("Quarterly sales")
  ->setCategoryLabels(["Q1", "Q2", "Q3"]);

$svg = $c->renderSvg();

// Basic shape.
var_dump(is_string($svg));
var_dump(strlen($svg) > 500);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(strpos($svg, "<svg ") !== false);
var_dump(str_ends_with(trim($svg), "</svg>"));

// Round-trip parse.
libxml_use_internal_errors(true);
$xml = simplexml_load_string($svg);
var_dump($xml instanceof SimpleXMLElement);

// Element counts: 3 categories x 2 series = 6 bars; plus frame /
// plot-bg rects. At least 8 <rect> expected. Title + axis tick
// labels + category labels produce multiple <text> nodes.
$rect_n = substr_count($svg, "<rect");
$text_n = substr_count($svg, "<text");
echo "rect>=8: ", ($rect_n >= 8 ? "yes" : "no ($rect_n)"), "\n";
echo "text>=4: ", ($text_n >= 4 ? "yes" : "no ($text_n)"), "\n";

// Title text present.
var_dump(strpos($svg, "Quarterly sales") !== false);

// Fragment-wrapped in <g class="fastchart">.
var_dump(strpos($svg, '<g class="fastchart">') !== false);

// Viewport reflects setSize().
var_dump(strpos($svg, 'width="480"') !== false);
var_dump(strpos($svg, 'height="280"') !== false);
var_dump(strpos($svg, 'viewBox="0 0 480 280"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
rect>=8: yes
text>=4: yes
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
OK
