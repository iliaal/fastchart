--TEST--
Review-round 3: setDpi bounds + scaling, JPEG/WebP quality 0, scatter error bars > 2048, format-width caps
--EXTENSIONS--
fastchart
--FILE--
<?php

/* setDpi() bounds: [24, 1200] inclusive. */
function dpi_throws($n) {
    try {
        (new FastChart\LineChart(100, 80))->setDpi($n);
        return "no throw";
    } catch (\ValueError $e) {
        return "ValueError";
    }
}
echo "dpi_bound_23:    ", dpi_throws(23),   "\n";    // reject
echo "dpi_bound_24:    ", dpi_throws(24),   "\n";    // accept
echo "dpi_bound_1200:  ", dpi_throws(1200), "\n";    // accept
echo "dpi_bound_1201:  ", dpi_throws(1201), "\n";    // reject

/* setDpi() scales the rendered canvas: setSize(96, 48) at DPI 192
 * produces a 192x96 PNG. setSize(96, 48) at DPI 96 stays at 96x48. */
function rendered_size($dpi) {
    $bytes = (new FastChart\LineChart(96, 48))
        ->setDpi($dpi)
        ->setSeries([['data' => [1, 2, 3]]])
        ->renderPng();
    $im = imagecreatefromstring($bytes);
    return imagesx($im) . "x" . imagesy($im);
}
echo "dpi_size_96:     ", rendered_size(96),  "\n";   // 96x48
echo "dpi_size_192:    ", rendered_size(192), "\n";   // 192x96
echo "dpi_size_300:    ", rendered_size(300), "\n";   // 300x150

/* Physical canvas cap: setSize(65535, 65535) + setDpi(1200) would
 * allocate ~819188px per side; the 16384 cap rejects it. */
try {
    (new FastChart\LineChart(65535, 65535))
        ->setDpi(1200)
        ->setSeries([['data' => [1, 2]]])
        ->renderPng();
    echo "dpi_canvas_cap:  no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "dpi_canvas_cap:  ValueError\n";
}

/* Cap edge: 16384 exact must pass; 16385 must reject. */
echo "dpi_cap_16384:   ";
try {
    $im = imagecreatefromstring(
        (new FastChart\LineChart(16384, 1))
            ->setSeries([['data' => [1, 2]]])->renderPng());
    echo (imagesx($im) === 16384 ? "ok" : "bad " . imagesx($im)), "\n";
} catch (\Throwable $e) { echo "ValueError (unexpected)\n"; }

echo "dpi_cap_16385:   ";
try {
    (new FastChart\LineChart(16385, 1))
        ->setSeries([['data' => [1, 2]]])->renderPng();
    echo "no throw (unexpected)\n";
} catch (\ValueError $e) { echo "ValueError\n"; }

/* Tiny-canvas-clamp: setDpi(24) on 1x1 logical rounds to 0x0 if
 * unclamped — libgd would emit a warning then return NULL. The
 * resolver clamps each dimension to >= 1 so the render succeeds. */
$tiny = (new FastChart\LineChart(1, 1))
    ->setDpi(24)
    ->setSeries([['data' => [1, 2]]])
    ->renderPng();
$im = imagecreatefromstring($tiny);
echo "dpi_tiny_clamp:  ", (imagesx($im) >= 1 && imagesy($im) >= 1 ? "ok" : "bad"), "\n";

/* renderToFile JPEG quality 0 rejected; WebP / AVIF quality 0 accepted. */
$tmp_jpg  = tempnam(sys_get_temp_dir(), 'fc_q0_jpg_')  . '.jpg';
$tmp_webp = tempnam(sys_get_temp_dir(), 'fc_q0_webp_') . '.webp';
$chart = (new FastChart\LineChart(80, 40))
    ->setSeries([['data' => [1, 2, 3]]]);

try {
    $chart->renderToFile($tmp_jpg, 0);
    echo "renderToFile_jpg0:   no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "renderToFile_jpg0:   ValueError\n";
}

$ok = $chart->renderToFile($tmp_webp, 0);
echo "renderToFile_webp0:  ", ($ok > 0 ? "ok" : "bad ($ok)"), "\n";

@unlink($tmp_jpg);
@unlink($tmp_webp);

/* Scatter error bars at index > 2048: must be honored. The earlier
 * unconditional cap at FASTCHART_MAX_POINTS_PER_SERIES (2048) silently
 * dropped scatter bars in the [2048, 4096) range. */
$points = [];
for ($i = 0; $i < 4096; $i++) {
    $points[] = [(float)$i, (float)($i % 100)];
}
$errs = array_fill(0, 4096, [0.0, 0.0]);  /* zero err = no visible bar */
$errs[3000] = [50.0, 50.0];               /* big bar at index 3000 */

$with_err = (new FastChart\ScatterChart(800, 400))
    ->setPoints($points)
    ->setErrorBars($errs)
    ->renderPng();
$without_err = (new FastChart\ScatterChart(800, 400))
    ->setPoints($points)
    ->renderPng();
echo "scatter_err_high: ", ($with_err !== $without_err ? "yes" : "no"), "\n";

/* Format-width / precision caps: 1..3 digit width and precision are
 * accepted; 4+ digits rejected. The hot path is snprintf in label
 * rendering; an unbounded width turned a single label into a
 * multi-second CPU burn. */
function format_throws($fmt) {
    try {
        (new FastChart\LineChart(100, 80))->setYAxisLabelFormat($fmt);
        return "no throw";
    } catch (\ValueError $e) {
        return "ValueError";
    }
}
echo "fmt_999_width:    ", format_throws('%999f'),       "\n";   // accept
echo "fmt_1000_width:   ", format_throws('%1000f'),      "\n";   // reject
echo "fmt_500M_width:   ", format_throws('%500000000f'), "\n";   // reject
echo "fmt_999_prec:     ", format_throws('%.999f'),      "\n";   // accept
echo "fmt_1000_prec:    ", format_throws('%.1000f'),     "\n";   // reject
?>
--EXPECT--
dpi_bound_23:    ValueError
dpi_bound_24:    no throw
dpi_bound_1200:  no throw
dpi_bound_1201:  ValueError
dpi_size_96:     96x48
dpi_size_192:    192x96
dpi_size_300:    300x150
dpi_canvas_cap:  ValueError
dpi_cap_16384:   ok
dpi_cap_16385:   ValueError
dpi_tiny_clamp:  ok
renderToFile_jpg0:   ValueError
renderToFile_webp0:  ok
scatter_err_high: yes
fmt_999_width:    no throw
fmt_1000_width:   ValueError
fmt_500M_width:   ValueError
fmt_999_prec:     no throw
fmt_1000_prec:    ValueError
