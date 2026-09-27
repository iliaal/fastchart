--TEST--
JPEG rendering honors max_execution_time after a successful render
--EXTENSIONS--
fastchart
--INI--
hard_timeout=0
--FILE--
<?php
require __DIR__ . '/_encoder_timeout.inc';
ini_set('memory_limit', '512M');
register_shutdown_function(static function (): void {});
$measurements = [];
foreach ([40, 160, 640, 2560, 4096] as $count) {
    $points = [];
    for ($i = 0; $i < $count; $i++) $points[] = (($i * 7919) % 1000) / 1000;
    $chart = (new FastChart\LineChart(4096, 4096))
        ->setSeries([['name' => 's', 'data' => $points]]);
    $jpeg = $chart->renderJpeg();
    if ($count === 40) {
        var_dump(str_starts_with($jpeg, "\xFF\xD8\xFF") && str_ends_with($jpeg, "\xFF\xD9"));
    }
    $times = [];
    for ($i = 0; $i < 2; $i++) {
        $start = fastchartTimeoutClock();
        $chart->renderJpeg();
        $times[] = fastchartTimeoutClock() - $start;
    }
    $measurements[$count] = $times;
    /* A short render can finish inside the platform timer's delivery slack. */
    if (min($times) >= 0.5) break;
}
if (min($times) < 0.5) {
    printf("insufficient render duration: %s\n", json_encode($measurements));
    exit(1);
}
$budget = min($times) * 0.25;
$limit = (int) ceil($budget) + 1;
ini_set('max_execution_time', (string) $limit);
$started = fastchartTimeoutClock();
$stop = $started + $limit - $budget;
while (fastchartTimeoutClock() < $stop) {}
echo "rendering\n";
$renderStart = fastchartTimeoutClock();
$chart->renderJpeg();
printf("not reached: clock=%s points=%d times=%s budget=%.4f elapsed=%.4f render=%.4f\n",
    fastchartTimeoutUsesWallClock() ? 'wall' : 'cpu', $count, json_encode($times),
    $budget, fastchartTimeoutClock() - $started,
    fastchartTimeoutClock() - $renderStart);
?>
--EXPECTF--
bool(true)
rendering

Fatal error: Maximum execution time of %d seconds exceeded in %s on line %d
