<?php
/* Discrete heatmap: each cell coloured directly from a value→ramp
 * interpolation. Distinct from ContourChart, which interpolates
 * isolines through the same input shape. Common use: hourly /
 * daily activity matrices, correlation matrices, calendar views. */

require __DIR__ . '/_bootstrap.php';

/* Synthetic 7-day x 24-hour traffic matrix with a clear weekday-
 * morning + weekend-evening pattern. */
$days  = ['Mon','Tue','Wed','Thu','Fri','Sat','Sun'];
$grid  = [];
for ($d = 0; $d < 7; $d++) {
    $row = [];
    for ($h = 0; $h < 24; $h++) {
        $weekday = $d < 5 ? 1 : 0;
        $morning = exp(-pow($h - 9, 2) / 12);
        $evening = exp(-pow($h - 21, 2) / 8);
        $base = 20
              + 80 * $weekday * $morning
              + 60 * (1 - $weekday) * $evening;
        $row[] = $base + sin($d + $h) * 4;
    }
    $grid[] = $row;
}

(new FastChart\Heatmap(720, 280))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Hourly traffic — last week')
    ->setGrid($grid)
    ->setColorRamp(0xF1F5F9, 0xE34A6F)   /* near-white → warm red */
    ->renderToFile(__DIR__ . '/35_heatmap.png');
