--TEST--
fastchart.max_render_pixels lowers the raster pixel budget
--EXTENSIONS--
fastchart
--INI--
fastchart.max_render_pixels=1000000
--FILE--
<?php

/* The rasterizer allocates ~8 bytes/pixel across two full frames, one
 * of them malloc-backed and outside memory_limit. The INI is the
 * operator-enforced ceiling; PHP_INI_SYSTEM so a script can't raise
 * it back. Built-in 64M cap still applies above the INI. */

echo "ini value: ", ini_get('fastchart.max_render_pixels'), "\n";
echo "ini_set blocked: ",
    ini_set('fastchart.max_render_pixels', '99999999') === false ? "ok" : "FAIL", "\n";

$c = (new FastChart\LineChart(2000, 2000))->setSeries([1, 2, 3]);
try {
    $c->renderPng();
    echo "4M pixels: NO THROW\n";
} catch (\ValueError $e) {
    echo "4M pixels rejected: ",
        str_contains($e->getMessage(), 'exceeds the 1000000 budget')
        && str_contains($e->getMessage(), 'fastchart.max_render_pixels')
        ? "ok" : $e->getMessage(), "\n";
}

// Under budget still renders; SVG output is vector and unaffected.
$small = (new FastChart\LineChart(900, 900))->setSeries([1, 2, 3]);
echo "0.81M pixels renders: ", strlen($small->renderPng()) > 100 ? "ok" : "FAIL", "\n";
echo "svg unaffected: ", strlen($c->renderSvg()) > 100 ? "ok" : "FAIL", "\n";

// The static svgTo*() conversions share the frame-buffer path and must
// honor the same ceiling — 1100x910 = 1,001,000 pixels, just over.
$overSvg  = (new FastChart\LineChart(1100, 910))->setSeries([1, 2, 3])->renderSvg();
$underSvg = (new FastChart\LineChart(900, 900))->setSeries([1, 2, 3])->renderSvg();
foreach (['svgToPng', 'svgToJpeg', 'svgToWebp'] as $m) {
    try {
        FastChart\Chart::$m($overSvg);
        echo "$m over budget: NO THROW\n";
    } catch (\ValueError $e) {
        echo "$m over budget rejected: ",
            str_contains($e->getMessage(), 'fastchart.max_render_pixels')
            ? "ok" : $e->getMessage(), "\n";
    }
    echo "$m under budget: ",
        strlen(FastChart\Chart::$m($underSvg)) > 100 ? "ok" : "FAIL", "\n";
}
?>
--EXPECT--
ini value: 1000000
ini_set blocked: ok
4M pixels rejected: ok
0.81M pixels renders: ok
svg unaffected: ok
svgToPng over budget rejected: ok
svgToPng under budget: ok
svgToJpeg over budget rejected: ok
svgToJpeg under budget: ok
svgToWebp over budget rejected: ok
svgToWebp under budget: ok
