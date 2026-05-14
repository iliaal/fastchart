--TEST--
Chart::drawSvgFragment(): emits a <g> group with no outer <svg>
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
$c = new FastChart\LineChart(320, 200);
$c->setSeries([10, 20, 15, 25, 18])
  ->setTitle("frag");

$c->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
$frag = $c->drawSvgFragment();
var_dump(is_string($frag));
var_dump(strlen($frag) > 200);

// No outer envelope.
var_dump(strpos($frag, "<?xml") === false);
var_dump(strpos($frag, "<svg ") === false);
var_dump(strpos($frag, "</svg>") === false);

// Starts with <g and ends with </g>.
$trimmed = trim($frag);
var_dump(str_starts_with($trimmed, "<g"));
var_dump(str_ends_with($trimmed, "</g>"));

// Title must still appear inside the group.
var_dump(strpos($frag, "frag") !== false);

// And the same chart's full SVG must wrap that same fragment.
$full = $c->renderSvg();
var_dump(str_starts_with($full, "<?xml "));
var_dump(strpos($full, "<g class=\"fastchart\">") !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
OK
