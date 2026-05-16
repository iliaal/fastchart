--TEST--
Chart::setSvgTextMode(): SVG_TEXT_PATHS flattens text to glyph paths
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc.php';
$font = fc_pick_font();
if ($font === '') die("skip no system font found\n");

function build_chart(string $font, int $mode): FastChart\LineChart {
    $c = new FastChart\LineChart(400, 200);
    $c->setFontPath($font);
    $c->setSvgTextMode($mode);
    $c->setTitle("Hello, world");
    $c->setSeries([['data' => [1, 2, 3, 4]]]);
    return $c;
}

// PATHS (default): every <text> flattened to <g><path d="…"/></g>;
// the title string itself does NOT appear as text content because
// glyphs are encoded as outlines.
$svg_paths = build_chart($font, FastChart\Chart::SVG_TEXT_PATHS)->renderSvg();
var_dump(is_string($svg_paths));
var_dump(strpos($svg_paths, "<text") === false);                    // no <text> elements
var_dump(strpos($svg_paths, '<g transform="translate(') !== false); // glyph-group wrapper
var_dump(strpos($svg_paths, '<path d="M') !== false);                // path data
var_dump(strpos($svg_paths, "Hello, world") === false);             // title is now outlines

// NATIVE: classic <text> output. Title text and font-family attrs back.
$svg_native = build_chart($font, FastChart\Chart::SVG_TEXT_NATIVE)->renderSvg();
var_dump(is_string($svg_native));
var_dump(strpos($svg_native, "<text") !== false);
var_dump(strpos($svg_native, "Hello, world") !== false);
var_dump(strpos($svg_native, "font-family=") !== false);

// PATHS files are bigger than NATIVE for the same chart.
var_dump(strlen($svg_paths) > strlen($svg_native));

// Default mode is PATHS.
$c2 = new FastChart\LineChart(200, 100);
$c2->setFontPath($font);
$c2->setSeries([['data' => [1, 2]]]);
$svg_default = $c2->renderSvg();
var_dump(strpos($svg_default, "<text") === false);

// Bad mode raises ValueError.
try {
    $c2->setSvgTextMode(99);
    echo "no throw\n";
} catch (ValueError $e) {
    echo "caught: ", $e->getMessage(), "\n";
}

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
bool(true)
caught: FastChart\Chart::setSvgTextMode() expects Chart::SVG_TEXT_PATHS or Chart::SVG_TEXT_NATIVE
OK
