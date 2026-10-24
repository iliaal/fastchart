<?php
/* Run with the baseline or candidate extension loaded. Reports median
 * microseconds per setHierarchy call. Build input outside the timed region;
 * reuse the chart so timings include releasing the previously parsed tree. */
function balanced(int $depth): array {
    return $depth ? ['children' => [balanced($depth - 1), balanced($depth - 1)]] : ['value' => 1];
}
$deep = ['children' => array_fill(0, 2016, ['value' => 1])];
for ($i = 0; $i < 31; $i++) $deep = ['children' => [$deep]];
$cases = [
    'wide-2048' => ['children' => array_fill(0, 2047, ['value' => 1])],
    'deep-2048' => $deep,
    'balanced-2047' => balanced(10),
];
$chart = new FastChart\SunburstChart(400, 400);
foreach ($cases as $name => $root) {
    for ($i = 0; $i < 100; $i++) $chart->setHierarchy($root);
    $times = [];
    for ($sample = 0; $sample < 9; $sample++) {
        $start = hrtime(true);
        for ($i = 0; $i < 1000; $i++) $chart->setHierarchy($root);
        $times[] = (hrtime(true) - $start) / 1000 / 1000;
    }
    sort($times);
    printf("%s %.3f %s\n", $name, $times[4], hash('sha256', $chart->renderSvg()));
}
