--TEST--
LineChart maps a secondary Y axis on the log scale, matching AreaChart
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression (fnd_690614b4): under a log Y scale LineChart computed the
 * secondary (right) axis range linearly, silently ignoring the log scale
 * that the primary axis and AreaChart both honor. It now uses the log
 * range for the secondary axis too and rejects non-positive secondary
 * data, consistent with AreaChart. */

/* Non-positive data on a log secondary axis must be rejected, not
 * silently linearized. */
$threw = false;
try {
    (new FastChart\LineChart(400, 300))
        ->setSeries([
            ['name' => 'L', 'data' => [1, 2, 3]],
            ['name' => 'R', 'data' => [-5, 10, 20], 'axis' => 'right'],
        ])
        ->setYAxisScale(FastChart\Chart::SCALE_LOG)
        ->renderSvg();
} catch (\Throwable $e) {
    $threw = true;
}
echo "rejects non-positive secondary: ", $threw ? "yes" : "no", "\n";

/* Strictly-positive secondary data renders fine on the log scale. */
$svg = (new FastChart\LineChart(400, 300))
    ->setSeries([
        ['name' => 'L', 'data' => [1, 2, 3]],
        ['name' => 'R', 'data' => [10, 100, 1000], 'axis' => 'right'],
    ])
    ->setYAxisScale(FastChart\Chart::SCALE_LOG)
    ->renderSvg();
echo "positive secondary renders: ", strlen($svg) > 100 ? "yes" : "no", "\n";

?>
--EXPECT--
rejects non-positive secondary: yes
positive secondary renders: yes
