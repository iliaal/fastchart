--TEST--
QrCode::renderSvg(): structural validation of vector output
--EXTENSIONS--
fastchart
gd
simplexml
--INI--
asan.detect_leaks=0
--FILE--
<?php
$q = (new FastChart\QrCode())
    ->setData('https://example.com')
    ->setSize(290, 290)
    ->setEcc(FastChart\QrCode::ECC_M);

$q->setSvgTextMode(FastChart\Symbol::SVG_TEXT_NATIVE);
$svg = $q->renderSvg();

// Basic shape: non-empty, XML prolog + <svg> root.
var_dump(is_string($svg));
var_dump(strlen($svg) > 1000);
var_dump(str_starts_with($svg, "<?xml "));
var_dump(strpos($svg, "<svg ") !== false);
var_dump(str_ends_with(trim($svg), "</svg>"));

// Round-trip parse: must be valid XML.
libxml_use_internal_errors(true);
$xml = simplexml_load_string($svg);
var_dump($xml instanceof SimpleXMLElement);

// QrCode emits dark modules as filled <rect>s (coalesced into runs
// along the x-axis). Even a short payload at version 1..3 produces
// a few dozen rect elements; longer payloads scale up. Lower-bound
// at 30 — sits comfortably below any realistic QR output.
$rect_n = substr_count($svg, "<rect");
echo "rect>=30: ", ($rect_n >= 30 ? "yes" : "no ($rect_n)"), "\n";

// Viewport: width / height / viewBox must reflect setSize().
var_dump(strpos($svg, 'width="290"') !== false);
var_dump(strpos($svg, 'height="290"') !== false);
var_dump(strpos($svg, 'viewBox="0 0 290 290"') !== false);

// Fragment mode: no envelope, just the <g> wrapper.
$frag = $q->drawSvgFragment();
var_dump(is_string($frag));
var_dump(!str_starts_with($frag, "<?xml "));
var_dump(str_starts_with($frag, "<g "));
var_dump(str_ends_with(trim($frag), "</g>"));

// renderToFile .svg routes to the SVG backend and writes valid SVG.
$tmp = tempnam(sys_get_temp_dir(), "fc_qr_svg_") . ".svg";
try {
    $written = $q->renderToFile($tmp);
    var_dump(is_int($written));
    var_dump($written > 1000);
    var_dump(file_exists($tmp));
    var_dump(filesize($tmp) === $written);

    $svg2 = file_get_contents($tmp);
    var_dump(str_starts_with($svg2, "<?xml "));
    var_dump(simplexml_load_string($svg2) instanceof SimpleXMLElement);
    var_dump($svg2 === $svg);
} finally {
    @unlink($tmp);
}

// Larger payload (auto-picks higher QR version, more modules).
$big = (new FastChart\QrCode())
    ->setData(str_repeat('ABC123 ', 14))
    ->setSize(400, 400);
$big->setSvgTextMode(FastChart\Symbol::SVG_TEXT_NATIVE);
$svgb = $big->renderSvg();
var_dump(simplexml_load_string($svgb) instanceof SimpleXMLElement);
// Larger QR symbols should have many more module rects.
var_dump(substr_count($svgb, "<rect") > $rect_n);

// Default canvas size when setSize() is not called: 300x300.
$svgd = (new FastChart\QrCode())->setData('default')->renderSvg();
var_dump(strpos($svgd, 'viewBox="0 0 300 300"') !== false);

echo "OK\n";
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
rect>=30: yes
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
