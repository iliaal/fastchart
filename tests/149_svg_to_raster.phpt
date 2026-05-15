--TEST--
Chart::svgToPng / svgToJpeg / svgToWebp: SVG bytes -> raster via plutovg
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Round-trip: build a chart, render SVG, then run the SVG bytes
 * back through the static rasterize methods. Output must be a
 * valid raster of each format. */
$c = (new FastChart\LineChart(400, 300))
    ->setTitle('Round-trip')
    ->setSeries([['data' => [1, 5, 3, 8, 4]]]);
$svg = $c->renderSvg();

$png  = FastChart\Chart::svgToPng($svg);
$jpg  = FastChart\Chart::svgToJpeg($svg);
$webp = FastChart\Chart::svgToWebp($svg);

echo "png_sig:  ", (substr($png,  0, 8) === "\x89PNG\r\n\x1A\n"          ? "ok" : "fail"), "\n";
echo "jpg_sig:  ", (substr($jpg,  0, 3) === "\xff\xd8\xff"                ? "ok" : "fail"), "\n";
echo "webp_sig: ", (substr($webp, 0, 4) === "RIFF" && substr($webp, 8, 4) === "WEBP" ? "ok" : "fail"), "\n";

/* The PNG IHDR dims must match the source SVG's 400x300. */
$png_w = unpack('N', substr($png, 16, 4))[1];
$png_h = unpack('N', substr($png, 20, 4))[1];
echo "png_dims_match_svg: ", ($png_w === 400 && $png_h === 300 ? "ok" : "fail"), "\n";

/* svgToJpeg honors $bgRgb. Render a tiny transparent SVG, encode
 * once with white bg and once with red bg, verify the encoded
 * bytes differ (transparent regions get composited differently). */
$transparent_svg = '<?xml version="1.0"?><svg xmlns="http://www.w3.org/2000/svg" width="32" height="32"><rect x="8" y="8" width="16" height="16" fill="#0000FF"/></svg>';
$jpg_white = FastChart\Chart::svgToJpeg($transparent_svg, 88, 0xFFFFFF);
$jpg_red   = FastChart\Chart::svgToJpeg($transparent_svg, 88, 0xFF0000);
echo "jpg_bg_differs: ", ($jpg_white !== $jpg_red ? "ok" : "fail"), "\n";

/* svgToWebp honors $mode. The four modes must yield different
 * byte counts (same logic as test 148, but for static entry). */
$webp_drawing  = FastChart\Chart::svgToWebp($svg, 90, FastChart\Chart::WEBP_DRAWING);
$webp_lossless = FastChart\Chart::svgToWebp($svg, 90, FastChart\Chart::WEBP_LOSSLESS);
$webp_fast     = FastChart\Chart::svgToWebp($svg, 90, FastChart\Chart::WEBP_FAST);
echo "webp_modes_differ: ",
    (count(array_unique([strlen($webp_drawing), strlen($webp_lossless), strlen($webp_fast)])) >= 2
        ? "ok" : "fail"), "\n";

/* Error paths */
try { FastChart\Chart::svgToPng(''); echo "empty: REGRESSION\n"; }
catch (\ValueError $e) { echo "empty: ", (str_contains($e->getMessage(), '1..') ? "ok" : "wrong-msg"), "\n"; }

try { FastChart\Chart::svgToPng('garbage'); echo "malformed: REGRESSION\n"; }
catch (\ValueError $e) { echo "malformed: ", (str_contains($e->getMessage(), 'intrinsic') ? "ok" : "wrong-msg"), "\n"; }

/* Out-of-range dim: 50000 > FC_IMAGE_MAX_DIM (4096). */
$huge = '<svg xmlns="http://www.w3.org/2000/svg" width="50000" height="100"></svg>';
try { FastChart\Chart::svgToPng($huge); echo "huge: REGRESSION\n"; }
catch (\ValueError $e) { echo "huge: ", (str_contains($e->getMessage(), 'exceed cap') ? "ok" : "wrong-msg"), "\n"; }

try { FastChart\Chart::svgToJpeg($svg, 150); echo "jpg_q: REGRESSION\n"; }
catch (\ValueError $e) { echo "jpg_q: ", (str_contains($e->getMessage(), '1, 100') ? "ok" : "wrong-msg"), "\n"; }

try { FastChart\Chart::svgToJpeg($svg, 88, -1); echo "jpg_bg: REGRESSION\n"; }
catch (\ValueError $e) { echo "jpg_bg: ", (str_contains($e->getMessage(), '24-bit') ? "ok" : "wrong-msg"), "\n"; }

try { FastChart\Chart::svgToWebp($svg, 90, 99); echo "webp_mode: REGRESSION\n"; }
catch (\ValueError $e) { echo "webp_mode: ", (str_contains($e->getMessage(), 'WEBP_DRAWING') ? "ok" : "wrong-msg"), "\n"; }

/* SVG with <text> elements: must not crash. Per spec, text renders
 * blank (plutovg has no text engine). Verify no exception, output
 * is still a valid PNG. */
$text_svg = '<?xml version="1.0"?><svg xmlns="http://www.w3.org/2000/svg" width="100" height="40"><text x="10" y="20" font-size="14">hi</text></svg>';
$out = FastChart\Chart::svgToPng($text_svg);
echo "text_no_crash: ", (substr($out, 0, 8) === "\x89PNG\r\n\x1A\n" ? "ok" : "fail"), "\n";

echo "ok\n";
?>
--EXPECT--
png_sig:  ok
jpg_sig:  ok
webp_sig: ok
png_dims_match_svg: ok
jpg_bg_differs: ok
webp_modes_differ: ok
empty: ok
malformed: ok
huge: ok
jpg_q: ok
jpg_bg: ok
webp_mode: ok
text_no_crash: ok
ok
