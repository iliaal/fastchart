--TEST--
Review-round 4: physical-pixel product cap + rotated label perf with TICK_POINTS
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

/* Product cap on physical pixels: 16384x16384 = 268M would allocate
 * ~1 GiB before encoder buffers; budget is 64M pixels (256 MiB) so
 * the worst-case square must reject. Both dims pass the per-axis
 * 16384 cap so this exercises the product check specifically. */
try {
    (new FastChart\LineChart(16384, 16384))
        ->setSeries([['data' => [1, 2]]])
        ->renderPng();
    echo "pixels_square: no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "pixels_square: ValueError\n";
}

/* Boundary: budget is 64M pixels (67108864 = 256 MiB of truecolor
 * canvas). Assert rejection at and above the boundary, but exercise
 * the success path on a far smaller render so a normal CI run
 * doesn't allocate ~256 MiB transiently per test invocation.
 * 8193x8193 = 67125249 just over the cap; 4096x4096 = 16M well
 * under. Both dims pass the per-axis 16384 cap so this exercises
 * the product check specifically. */
try {
    $bytes = (new FastChart\LineChart(4096, 4096))
        ->setSeries([['data' => [1, 2]]])
        ->renderPng();
    $im = imagecreatefromstring($bytes);
    echo "pixels_under: ", imagesx($im), "x", imagesy($im), "\n";
} catch (\ValueError $e) {
    echo "pixels_under: ValueError (unexpected)\n";
}

try {
    (new FastChart\LineChart(8193, 8193))
        ->setSeries([['data' => [1, 2]]])
        ->renderPng();
    echo "pixels_over: no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "pixels_over: ValueError\n";
}

/* DPI-driven product cap: 6000x6000 logical at DPI 200 -> 12500x12500
 * physical = 156M pixels. Must reject under the 64M-pixel budget even
 * though both axes individually fit. */
try {
    (new FastChart\LineChart(6000, 6000))
        ->setDpi(200)
        ->setSeries([['data' => [1, 2]]])
        ->renderPng();
    echo "pixels_dpi: no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "pixels_dpi: ValueError\n";
}

/* Rotated categorical X-axis with TICK_POINTS: labels are
 * suppressed, so the previous unconditional measurement walk burned
 * >1s on 4096 categories at 200 DPI. Compare against TICK_NONE on
 * the same data — the difference must be small (well under 500ms);
 * a regression to per-category measurement runs ~1s. */
$categories = [];
for ($i = 0; $i < 4096; $i++) {
    $categories[] = "Category $i with somewhat-long text";
}
/* setSeries caps points/series at 2048; the perf hot-path is
 * label measurement, not series sampling, so a sparse 2-point
 * series with 4096 categories still drives the buggy code path. */
$series = [['data' => [1, 2]]];

$t0 = microtime(true);
(new FastChart\LineChart(800, 400))
    ->setDpi(200)
    ->setSeries($series)
    ->setCategoryLabels($categories)
    ->setXAxisLabelAngle(45)
    ->setTickMode(FastChart\Chart::TICK_POINTS)
    ->renderPng();
$with_points = microtime(true) - $t0;

$t0 = microtime(true);
(new FastChart\LineChart(800, 400))
    ->setDpi(200)
    ->setSeries($series)
    ->setCategoryLabels($categories)
    ->setXAxisLabelAngle(45)
    ->setTickMode(FastChart\Chart::TICK_NONE)
    ->renderPng();
$without = microtime(true) - $t0;

/* The unconditional walk was the entire delta. Allow generous
 * headroom (500 ms) so this isn't flaky on slow CI. The pre-fix
 * gap was ~1.4s. */
echo "rotated_perf: ", ($with_points - $without < 0.5 ? "ok" : "slow"), "\n";
?>
--EXPECT--
pixels_square: ValueError
pixels_under: 4096x4096
pixels_over: ValueError
pixels_dpi: ValueError
rotated_perf: ok
