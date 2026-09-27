--TEST--
ScatterChart polynomial trend lines preserve exact cancellation at large Y scales
--EXTENSIONS--
fastchart
--FILE--
<?php
foreach ([0, 48, 50, 900] as $exponent) {
    $unit = 2.0 ** $exponent;
    $points = [];
    // Each X group sums to zero; five distinct X values give full rank through degree 3.
    foreach ([-1, -0.5, 0, 0.5, 1] as $x) {
        for ($i = 0; $i < 20; $i++) $points[] = [$x, $unit];
        $points[] = [$x, -10 * $unit];
        $points[] = [$x, -10 * $unit];
    }
    foreach ([1, 2, 3] as $degree) {
        $svg = (new FastChart\ScatterChart(500, 400))
            ->setPoints($points)
            ->setPlotRect(60, 20, 460, 320)
            ->setYAxisRange(-1, 1)
            ->setLegendPosition(FastChart\Chart::LEGEND_NONE)
            ->setTrendLine(true, 0xFF0000, $degree)
            ->renderSvg();
        preg_match_all('/<line x1="[-0-9.]+" y1="([-0-9.]+)" x2="[-0-9.]+" y2="([-0-9.]+)" stroke="#FF0000" stroke-width="2"/', $svg, $lines, PREG_SET_ORDER);
        $ok = count($lines) === 200;
        foreach ($lines as $line) {
            $ok = $ok && (float)$line[1] === 170.0 && (float)$line[2] === 170.0;
        }
        echo "2^$exponent degree $degree: ", $ok ? 'ok' : 'BAD', "\n";
    }
}
?>
--EXPECT--
2^0 degree 1: ok
2^0 degree 2: ok
2^0 degree 3: ok
2^48 degree 1: ok
2^48 degree 2: ok
2^48 degree 3: ok
2^50 degree 1: ok
2^50 degree 2: ok
2^50 degree 3: ok
2^900 degree 1: ok
2^900 degree 2: ok
2^900 degree 3: ok
