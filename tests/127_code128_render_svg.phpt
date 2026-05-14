--TEST--
Code128::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
$c = (new FastChart\Code128())
    ->setData('HELLO')
    ->setSize(300, 80)
    ->setForeground(0x000000)
    ->setBackground(0xFFFFFF);

$svg = $c->renderSvg();

// Basic shape: non-empty, XML prolog + <svg> root.
var_dump(is_string($svg));
var_dump(strlen($svg) > 200);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(strpos($svg, "<svg ") !== false);
var_dump(str_ends_with(trim($svg), "</svg>"));

// Round-trip parse: must be valid XML.
libxml_use_internal_errors(true);
$xml = simplexml_load_string($svg);
var_dump($xml instanceof SimpleXMLElement);

// Code128 emits bars as filled <rect> elements; HELLO + start/check/
// stop produces a healthy number of bar rects. The opaque bg also
// contributes one rect. Lower-bound at 20 — a structurally correct
// Code128 sits comfortably above this.
$rect_n = substr_count($svg, "<rect");
echo "rect>=20: ", ($rect_n >= 20 ? "yes" : "no ($rect_n)"), "\n";

// Viewport: width / height / viewBox must reflect setSize().
var_dump(strpos($svg, 'width="300"') !== false);
var_dump(strpos($svg, 'height="80"') !== false);
var_dump(strpos($svg, 'viewBox="0 0 300 80"') !== false);

// Fragment mode: no envelope, just the <g> wrapper.
$frag = $c->drawSvgFragment();
var_dump(is_string($frag));
var_dump(!str_starts_with($frag, "<?xml "));
var_dump(str_starts_with($frag, "<g "));
var_dump(str_ends_with(trim($frag), "</g>"));

// renderToFile .svg routes to the SVG backend and writes valid SVG.
$tmp = tempnam(sys_get_temp_dir(), "fc_c128_svg_") . ".svg";
try {
    $written = $c->renderToFile($tmp);
    var_dump(is_int($written));
    var_dump($written > 200);
    var_dump(file_exists($tmp));
    var_dump(filesize($tmp) === $written);

    $svg2 = file_get_contents($tmp);
    var_dump(str_starts_with($svg2, "<?xml "));
    var_dump(simplexml_load_string($svg2) instanceof SimpleXMLElement);
    var_dump($svg2 === $svg);

    // Case-insensitive extension match: .SVG also works.
    $tmp_upper = $tmp . "x.SVG";
    $w2 = $c->renderToFile($tmp_upper);
    var_dump($w2 > 0);
    var_dump(file_get_contents($tmp_upper) === $svg);
    unlink($tmp_upper);
} finally {
    @unlink($tmp);
}

// show_text path emits a native <text> element on SVG (instead of
// rasterising glyphs into the canvas).
$ct = (new FastChart\Code128())
    ->setData('FASTCHART-12345')
    ->setSize(400, 100)
    ->setShowText(true);
$svgt = $ct->renderSvg();
var_dump(substr_count($svgt, "<text") >= 1);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
rect>=20: yes
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
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
OK
