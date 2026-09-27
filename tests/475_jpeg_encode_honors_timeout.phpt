--TEST--
JPEG rendering honors max_execution_time after a successful render
--EXTENSIONS--
fastchart
--FILE--
<?php
require __DIR__ . '/_encoder_timeout.inc';
ini_set('memory_limit', '512M');
register_shutdown_function(static function (): void {});
$points = [];
for ($i = 0; $i < 40; $i++) $points[] = (($i * 7919) % 1000) / 1000;
$chart = (new FastChart\LineChart(4096, 4096))
    ->setSeries([['name' => 's', 'data' => $points]]);
$jpeg = $chart->renderJpeg();
var_dump(str_starts_with($jpeg, "\xFF\xD8\xFF") && str_ends_with($jpeg, "\xFF\xD9"));
$times = [];
for ($i = 0; $i < 2; $i++) {
    $start = fastchartTimeoutClock();
    $chart->renderJpeg();
    $times[] = fastchartTimeoutClock() - $start;
}
$budget = min($times) * 0.25;
$limit = (int) ceil($budget) + 1;
ini_set('max_execution_time', (string) $limit);
$stop = fastchartTimeoutClock() + $limit - $budget;
while (fastchartTimeoutClock() < $stop) {}
echo "rendering\n";
$chart->renderJpeg();
echo "not reached\n";
?>
--EXPECTF--
bool(true)
rendering

Fatal error: Maximum execution time of %d seconds exceeded in %s on line %d
