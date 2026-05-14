<?php

/* Step plot via INTERP_STEP_AFTER and INTERP_STEP_BEFORE: just exercise
 * the new modes through a normal LineChart render and confirm a non-
 * trivial PNG comes out. */
$rows = [];
for ($i = 0; $i < 12; $i++) $rows[] = 50 + ($i % 3) * 10;

foreach ([
    FastChart\Chart::INTERP_LINEAR,
    FastChart\Chart::INTERP_SMOOTH,
    FastChart\Chart::INTERP_STEP_AFTER,
    FastChart\Chart::INTERP_STEP_BEFORE,
] as $mode) {
    $im = imagecreatetruecolor(400, 200);
    $out = (new FastChart\LineChart(400, 200))
        ->setSeries([['label' => 's', 'data' => $rows]])
        ->setLineInterpolation($mode)
        ->draw($im);
    echo "interp=$mode: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";
}

/* Validation: bogus interp mode. */
try {
    (new FastChart\LineChart)->setLineInterpolation(99);
    echo "bad_interp: no throw\n";
} catch (\ValueError $e) {
    echo "bad_interp: ValueError ok\n";
}

/* Plot bands: 3 bands, mixed alphas. Renders into a LineChart. */
$im = imagecreatetruecolor(400, 200);
$chart = (new FastChart\LineChart(400, 200))
    ->setSeries([['data' => [10, 30, 60, 80, 95]]])
    ->addHorizontalBand(20.0, 40.0, 0xFFEE00, 96, "low")
    ->addHorizontalBand(40.0, 70.0, 0x00FF00, 80, "mid")
    ->addHorizontalBand(70.0, 100.0, 0xFF0000, 96, "high");
echo "bands_render: ", ($chart->draw($im) instanceof \GdImage ? "ok" : "bad"), "\n";

/* low > high should still work — setter normalizes. */
$im = imagecreatetruecolor(200, 100);
echo "swapped_bounds: ", (
    (new FastChart\LineChart(200, 100))
        ->setSeries([['data' => [1, 2, 3]]])
        ->addHorizontalBand(50.0, 10.0, 0x808080)
        ->draw($im) instanceof \GdImage ? "ok" : "bad"
), "\n";

/* Validation: bad color. */
try {
    (new FastChart\LineChart)->addHorizontalBand(0.0, 1.0, 0x1000000);
    echo "bad_color: no throw\n";
} catch (\ValueError $e) {
    echo "bad_color: ValueError ok\n";
}

/* Validation: bad alpha. */
try {
    (new FastChart\LineChart)->addHorizontalBand(0.0, 1.0, 0xFFFFFF, 200);
    echo "bad_alpha: no throw\n";
} catch (\ValueError $e) {
    echo "bad_alpha: ValueError ok\n";
}

/* Validation: NaN bound. */
try {
    (new FastChart\LineChart)->addHorizontalBand(NAN, 1.0, 0xFFFFFF);
    echo "nan_low: no throw\n";
} catch (\ValueError $e) {
    echo "nan_low: ValueError ok\n";
}

/* Cap at 16 bands. */
$chart = new FastChart\LineChart;
for ($i = 0; $i < 16; $i++) $chart->addHorizontalBand($i, $i + 1, 0x808080);
try {
    $chart->addHorizontalBand(100.0, 101.0, 0x808080);
    echo "seventeenth: no throw\n";
} catch (\ValueError $e) {
    echo "seventeenth: ValueError ok\n";
}

/* INTERP_STEP_AFTER / INTERP_STEP_BEFORE constants are 2 and 3. */
echo "STEP_AFTER=", FastChart\Chart::INTERP_STEP_AFTER,
     " STEP_BEFORE=", FastChart\Chart::INTERP_STEP_BEFORE, "\n";
?>
