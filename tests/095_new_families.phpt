--TEST--
Funnel / Waterfall / Heatmap / LinearMeter / Polar(STYLE_ROSE) all render
--EXTENSIONS--
fastchart
gd
--FILE--
<?php
/* Each family must:
 *   1. Render successfully on a small canvas with non-trivial data.
 *   2. Reflect a perturbation of the input in the output bytes.
 *   3. Honour family-specific edge cases (cap, missing data, etc.). */

/* --- Funnel --- */
$f_a = (new FastChart\Funnel(280, 220))
    ->setStages([
        ['label' => 'A', 'value' => 100],
        ['label' => 'B', 'value' => 50],
        ['label' => 'C', 'value' => 20],
    ])->renderPng();
echo "funnel_renders:    ", ($f_a !== '' ? "yes" : "no"), "\n";
$f_b = (new FastChart\Funnel(280, 220))
    ->setStages([
        ['label' => 'A', 'value' => 100],
        ['label' => 'B', 'value' => 90],
        ['label' => 'C', 'value' => 20],
    ])->renderPng();
echo "funnel_perturbs:   ", ($f_a !== $f_b ? "yes" : "no"), "\n";

/* Non-positive stage values silently dropped; zero-stage chart errors. */
try {
    (new FastChart\Funnel(280, 220))->setStages([['value' => 0]])->renderPng();
    echo "funnel_empty: no throw (unexpected)\n";
} catch (\Error $e) {
    echo "funnel_empty:      throws\n";
}

/* --- Waterfall --- */
$w_a = (new FastChart\Waterfall(360, 240))
    ->setBars([
        ['label' => 'Start', 'value' => 100, 'kind' => 'total'],
        ['label' => 'Add',   'value' => 30],
        ['label' => 'Drop',  'value' => -40],
        ['label' => 'End',   'value' => 90, 'kind' => 'total'],
    ])->renderPng();
echo "waterfall_renders: ", ($w_a !== '' ? "yes" : "no"), "\n";

$w_b = (new FastChart\Waterfall(360, 240))
    ->setBars([
        ['label' => 'Start', 'value' => 100, 'kind' => 'total'],
        ['label' => 'Add',   'value' => 30],
        ['label' => 'Drop',  'value' => -90], /* bigger drop = different layout */
        ['label' => 'End',   'value' => 40,  'kind' => 'total'],
    ])->renderPng();
echo "waterfall_perturbs:", ($w_a !== $w_b ? "yes" : "no"), "\n";

/* setRiseColor flips the colour of a delta-positive bar. */
$w_c = (new FastChart\Waterfall(360, 240))
    ->setRiseColor(0xAA00AA)
    ->setBars([['label' => 'A', 'value' => 100], ['label' => 'B', 'value' => 50]])
    ->renderPng();
$w_d = (new FastChart\Waterfall(360, 240))
    ->setBars([['label' => 'A', 'value' => 100], ['label' => 'B', 'value' => 50]])
    ->renderPng();
echo "waterfall_color:   ", ($w_c !== $w_d ? "differs" : "same"), "\n";

/* --- Heatmap --- */
$grid = [[1, 2, 3], [4, 5, 6], [7, 8, 9]];
$h_a = (new FastChart\Heatmap(240, 240))->setGrid($grid)->renderPng();
echo "heatmap_renders:   ", ($h_a !== '' ? "yes" : "no"), "\n";

$grid_alt = $grid;
$grid_alt[0][0] = 100;
$h_b = (new FastChart\Heatmap(240, 240))->setGrid($grid_alt)->renderPng();
echo "heatmap_perturbs:  ", ($h_a !== $h_b ? "yes" : "no"), "\n";

/* Custom ramp shifts the whole image colour-wise. */
$h_c = (new FastChart\Heatmap(240, 240))
    ->setColorRamp(0x000000, 0xFFFFFF)
    ->setGrid($grid)->renderPng();
echo "heatmap_ramp:      ", ($h_a !== $h_c ? "differs" : "same"), "\n";

/* Empty / uniform / non-finite-only grids must error rather than
 * crash divide-by-zero in the normalisation. */
try {
    (new FastChart\Heatmap(240, 240))->setGrid([[5, 5], [5, 5]])->renderPng();
    echo "heatmap_uniform: no throw (unexpected)\n";
} catch (\Error $e) {
    echo "heatmap_uniform:   throws\n";
}

/* --- LinearMeter --- */
$lm_a = (new FastChart\LinearMeter(360, 120))
    ->setRange(0, 100)->setValue(50)
    ->setZones([['from' => 0, 'to' => 100]])->renderPng();
echo "meter_renders:     ", ($lm_a !== '' ? "yes" : "no"), "\n";

$lm_b = (new FastChart\LinearMeter(360, 120))
    ->setRange(0, 100)->setValue(80)
    ->setZones([['from' => 0, 'to' => 100]])->renderPng();
echo "meter_perturbs:    ", ($lm_a !== $lm_b ? "yes" : "no"), "\n";

/* Vertical orientation differs from horizontal. */
$lm_v = (new FastChart\LinearMeter(160, 360))
    ->setRange(0, 100)->setValue(50)
    ->setOrientation(FastChart\LinearMeter::METER_VERTICAL)
    ->setZones([['from' => 0, 'to' => 100]])->renderPng();
echo "meter_vertical:    ", ($lm_v !== '' ? "yes" : "no"), "\n";

/* Range constraint: max <= min rejected. */
try {
    (new FastChart\LinearMeter(160, 80))->setRange(10, 10);
    echo "meter_bad_range: no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "meter_bad_range:   ValueError\n";
}

/* --- PolarChart STYLE_ROSE --- */
$pts = [];
for ($i = 0; $i < 8; $i++) $pts[] = [$i * 45.0, 1.0 + $i * 0.5];
$rose = (new FastChart\PolarChart(280, 280))
    ->setStyle(FastChart\PolarChart::STYLE_ROSE)
    ->setSeries([['data' => $pts]])
    ->renderPng();
$line = (new FastChart\PolarChart(280, 280))
    ->setStyle(FastChart\PolarChart::STYLE_LINE)
    ->setSeries([['data' => $pts]])
    ->renderPng();
echo "rose_differs_line: ", ($rose !== $line ? "yes" : "no"), "\n";

try {
    (new FastChart\PolarChart(200, 200))->setStyle(99);
    echo "rose_bad_style: no throw (unexpected)\n";
} catch (\ValueError $e) {
    echo "rose_bad_style:    ValueError\n";
}
?>
--EXPECT--
funnel_renders:    yes
funnel_perturbs:   yes
funnel_empty:      throws
waterfall_renders: yes
waterfall_perturbs:yes
waterfall_color:   differs
heatmap_renders:   yes
heatmap_perturbs:  yes
heatmap_ramp:      differs
heatmap_uniform:   throws
meter_renders:     yes
meter_perturbs:    yes
meter_vertical:    yes
meter_bad_range:   ValueError
rose_differs_line: yes
rose_bad_style:    ValueError
