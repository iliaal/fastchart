<?php
// Exercises the smart_str growth paths of a prebuilt module, so an import the
// running PHP does not export fails here: PHP loads extensions with RTLD_LAZY,
// and an unresolved function only aborts when first called.

function fail(string $msg): never
{
    fwrite(STDERR, "FAIL: $msg\n");
    exit(1);
}

if (!extension_loaded('fastchart')) {
    fail('fastchart is not loaded');
}

$series = [];
for ($i = 0; $i < 2000; $i++) {
    $series[] = round(sin($i / 40) * 100 + $i % 17, 3);
}
$chart = (new FastChart\LineChart(1200, 600))
    ->setTitle('Prebuilt ABI smoke')
    ->setSeries($series);

// The SVG document is assembled in a smart_str; 64 KiB forces it through
// repeated reallocation.
$svg = $chart->renderSvg();
if (!is_string($svg) || strlen($svg) < 64 * 1024 || !str_ends_with(rtrim($svg), '</svg>')) {
    fail('renderSvg() did not produce the expected large document');
}

// Raster output rasterizes that SVG and runs each statically linked codec.
foreach (['Png' => IMAGETYPE_PNG, 'Jpeg' => IMAGETYPE_JPEG, 'Webp' => IMAGETYPE_WEBP] as $format => $type) {
    $bytes = $chart->{"render$format"}();
    $info = is_string($bytes) ? getimagesizefromstring($bytes) : false;
    if ($info === false || $info[0] !== 1200 || $info[1] !== 600 || $info[2] !== $type) {
        fail("render$format() did not produce a 1200x600 image");
    }
}

printf("ok php=%s fastchart=%s svg_bytes=%d\n", PHP_VERSION, FastChart\Chart::version(), strlen($svg));
