--TEST--
PolarChart::renderSvg(): structural validation of vector output (rose + line)
--EXTENSIONS--
fastchart
gd
simplexml
--INI--
asan.detect_leaks=0
--FILE--
<?php
$pts = [];
for ($i = 0; $i < 8; $i++) $pts[] = [$i * 45.0, 1.0 + $i * 0.5];

$rose = (new FastChart\PolarChart(360, 360))
    ->setStyle(FastChart\PolarChart::STYLE_ROSE)
    ->setSeries([['data' => $pts]])
    ->setTitle('Polar Rose')
    ->renderSvg();

var_dump(is_string($rose));
var_dump(str_starts_with($rose, "<?xml "));
var_dump(str_ends_with(trim($rose), "</svg>"));
libxml_use_internal_errors(true);
var_dump(simplexml_load_string($rose) instanceof SimpleXMLElement);

// 8 wedges * (fill + outline) = at least 16 paths (arc renders as path).
$path_n = substr_count($rose, "<path");
echo "rose_path>=16: ", ($path_n >= 16 ? "yes" : "no ($path_n)"), "\n";

// Concentric grid: 4 rings as circles, plus radial spokes as lines.
$circ_n = substr_count($rose, "<circle");
echo "rose_circle>=4: ", ($circ_n >= 4 ? "yes" : "no ($circ_n)"), "\n";

$line = (new FastChart\PolarChart(360, 360))
    ->setStyle(FastChart\PolarChart::STYLE_LINE)
    ->setSeries([['data' => $pts]])
    ->setTitle('Polar Line')
    ->renderSvg();

var_dump(is_string($line));
var_dump(simplexml_load_string($line) instanceof SimpleXMLElement);

// STYLE_LINE: 8-vertex polygon emitted as <line> segments + markers as <circle>.
$line_n = substr_count($line, "<line");
echo "line_line>=8: ", ($line_n >= 8 ? "yes" : "no ($line_n)"), "\n";

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
rose_path>=16: yes
rose_circle>=4: yes
bool(true)
bool(true)
line_line>=8: yes
OK
