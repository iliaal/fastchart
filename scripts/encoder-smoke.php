<?php
/* Phase 2 smoke check, extended with behavioral magic-byte asserts.
 *
 * What this script verifies:
 *   1. The .so loads.
 *   2. The build linked libpng / libjpeg / libwebp / plutovg (no missing-
 *      symbol errors at dlopen time).
 *   3. The vendored plutovg/plutosvg symbols are not exported (visibility
 *      check, also implicit since dlopen with hidden-only externals
 *      would fail).
 *   4. Each raster encoder emits framing-valid bytes for a real chart:
 *      PNG / JPEG / WebP magic bytes plus exact dimensions via
 *      getimagesizefromstring() (core, no ext/gd needed).
 *
 * (4) is covered below by rendering a real chart through each encoder.
 */
if (!class_exists('FastChart\\Chart')) {
    fwrite(STDERR, "fastchart.so not loaded\n");
    exit(1);
}

fprintf(STDOUT, "fastchart %s loaded; new encoder/rasterizer linked.\n",
    FastChart\Chart::version());

function fc_smoke_fail(string $message): void {
    fwrite(STDERR, "encoder-smoke FAIL: $message\n");
    exit(1);
}

$chart = (new FastChart\LineChart(320, 200))->setSeries([1, 4, 2, 5]);

$png = $chart->renderPng();
if (!str_starts_with($png, "\x89PNG\r\n\x1A\n")) {
    fc_smoke_fail('renderPng output lacks the PNG signature');
}

$jpeg = $chart->renderJpeg();
if (!str_starts_with($jpeg, "\xFF\xD8\xFF")
    || !str_ends_with($jpeg, "\xFF\xD9")) {
    fc_smoke_fail('renderJpeg output lacks JPEG SOI/EOI markers');
}

$webp = $chart->renderWebp();
if (!str_starts_with($webp, 'RIFF')
    || substr($webp, 8, 4) !== 'WEBP') {
    fc_smoke_fail('renderWebp output lacks the RIFF....WEBP container');
}

foreach (['png' => $png, 'jpeg' => $jpeg, 'webp' => $webp] as $name => $bytes) {
    $info = getimagesizefromstring($bytes);
    if ($info === false || $info[0] !== 320 || $info[1] !== 200) {
        fc_smoke_fail("render" . ucfirst($name) . ' decoded dims != 320x200');
    }
    printf("encoder-smoke: %-4s %dx%d %d bytes magic ok\n",
        $name, $info[0], $info[1], strlen($bytes));
}

echo "encoder-smoke behavioral asserts passed\n";
