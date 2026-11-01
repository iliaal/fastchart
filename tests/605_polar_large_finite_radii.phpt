--TEST--
PolarChart: large finite radii retain proportional geometry in every render path
--EXTENSIONS--
fastchart
--FILE--
<?php
function polarScaled(float $scale, string $mode, bool $forced, bool $vectors = false, bool $tiny = false): string {
    $chart = (new FastChart\PolarChart(400, 400))
        ->setSeries([[0, $scale], [90, $scale / 2], [180, $scale / 4], [270, 0]])
        ->setFilled(str_contains($mode, 'filled'))
        ->setInterpolation(str_contains($mode, 'smooth')
            ? FastChart\Chart::INTERP_SMOOTH : FastChart\Chart::INTERP_LINEAR);
    if ($mode === 'rose') $chart->setStyle(FastChart\PolarChart::STYLE_ROSE);
    if ($forced) $chart->setMaxRadius($tiny ? 1e-300 : $scale * 2);
    if ($vectors) {
        $chart->addVectors([
            ['angle' => 0, 'radius' => $scale / 2,
             'angle_to' => 90, 'radius_to' => $scale],
            ['angle' => 180, 'radius' => $scale,
             'angle_to' => 270, 'radius_to' => $scale / 4],
        ]);
    }
    return $chart->renderSvg();
}
$modes = ['line', 'filled', 'smooth-line', 'smooth-filled', 'rose'];
foreach ($modes as $mode) {
    foreach ([false, true] as $forced) {
        foreach ([false, true] as $vectors) {
            $expected = polarScaled(1, $mode, $forced, $vectors);
            /* Binary scaling leaves the normalized ratios exact. */
            foreach ([2.0 ** 1018, 2.0 ** 1020, 2.0 ** 1022] as $scale) {
                if (polarScaled($scale, $mode, $forced, $vectors) !== $expected) {
                    echo "$mode forced=$forced vectors=$vectors geometry differs\n";
                }
            }
        }
    }
}
echo "proportional geometry checked\n";
/* Every positive radius exceeds the existing eight-plot-radius cap.
 * Overflow in division with a tiny forced maximum must still saturate. */
foreach ($modes as $mode) {
    foreach ([false, true] as $vectors) {
        if (polarScaled(2.0 ** 1022, $mode, true, $vectors, true)
            !== polarScaled(32, $mode, true, $vectors, true)) {
            echo "$mode vectors=$vectors clamp differs\n";
        }
    }
}
echo "tiny maximum checked\n";
?>
--EXPECT--
proportional geometry checked
tiny maximum checked
