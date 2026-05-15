<?php
/* WebP encoder modes via setWebpMode(). The four modes pick
 * different libwebp configurations for different output goals:
 *
 *   WEBP_DRAWING  (default): chart content. WEBP_PRESET_DRAWING +
 *                            method=2. Best speed/size balance.
 *   WEBP_PHOTO            : embedded photographic backgrounds.
 *                            WEBP_PRESET_PHOTO + method=4.
 *   WEBP_LOSSLESS         : bit-exact output. lossless=1, method=6.
 *                            Often smaller than lossy at q=90 on
 *                            chart content; slower encode.
 *   WEBP_FAST             : fastest encode at the cost of larger
 *                            files. WEBP_PRESET_DRAWING + method=0.
 *
 * This script renders one chart per mode and reports byte size +
 * median encode time so you can pick what fits your pipeline. */

require __DIR__ . '/_bootstrap.php';

$build = fn() => (new FastChart\LineChart(1280, 720))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Sample chart for WebP mode comparison')
    ->setSeries([
        ['label' => 'A', 'data' => [10, 20, 25, 18, 27, 35, 40, 52, 48, 60]],
        ['label' => 'B', 'data' => [ 6, 12, 16, 22, 28, 30, 33, 35, 38, 42]],
    ])
    ->setCategoryLabels(['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct']);

$modes = [
    'DRAWING'  => FastChart\Chart::WEBP_DRAWING,
    'PHOTO'    => FastChart\Chart::WEBP_PHOTO,
    'LOSSLESS' => FastChart\Chart::WEBP_LOSSLESS,
    'FAST'     => FastChart\Chart::WEBP_FAST,
];

printf("%-10s %10s %10s\n", 'mode', 'bytes', 'median_ms');
printf("%-10s %10s %10s\n", '----', '-----', '---------');

foreach ($modes as $name => $mode) {
    $c = $build()->setWebpMode($mode);

    /* Warm + sample. */
    for ($i = 0; $i < 3; $i++) $c->renderWebp();
    $times = [];
    $bytes = 0;
    for ($i = 0; $i < 15; $i++) {
        $t0 = hrtime(true);
        $w  = $c->renderWebp();
        $times[] = (hrtime(true) - $t0) / 1e6;
        $bytes   = strlen($w);
    }
    sort($times);
    $median = $times[(int)(count($times) * 0.5)];

    printf("%-10s %10d %10.2f\n", $name, $bytes, $median);

    /* Also dump one PNG per mode so you can eyeball the visual
     * result (no difference between lossy modes at q=90 for chart
     * content; LOSSLESS will be pixel-identical to the SVG render). */
    $c->renderToFile(__DIR__ . "/50_webp_{$name}.webp");
}
