<?php
function fastchartTimeoutClock(): float
{
    if (PHP_OS_FAMILY === 'Windows') return hrtime(true) / 1e9;
    $r = getrusage();
    return $r['ru_utime.tv_sec'] + $r['ru_utime.tv_usec'] / 1e6
        + $r['ru_stime.tv_sec'] + $r['ru_stime.tv_usec'] / 1e6;
}

function fastchartEncoderTimeout(string $format): void
{
    $dir = sys_get_temp_dir() . '/fastchart-encoder-' . $format . '-' . getmypid();
    mkdir($dir);
    $path = $dir . '/output.' . $format;
    $command = escapeshellarg(PHP_BINARY) . ' ' . getenv('TEST_PHP_ARGS')
        . ' -d memory_limit=512M -d hard_timeout=0 '
        . escapeshellarg(__FILE__) . ' --child ' . escapeshellarg($path);
    $output = [];
    exec($command . ' 2>&1', $output, $status);
    $text = implode("\n", $output);
    $files = array_values(array_diff(scandir($dir), ['.', '..']));
    $ok = str_contains($text, 'armed ')
        && str_contains($text, 'Maximum execution time')
        && !str_contains($text, 'returned')
        && $status === 255 && $files === [];
    if ($ok) {
        echo "timeout before file commit\n";
    } else {
        printf("status=%d files=%s\n%s\n", $status, json_encode($files), $text);
    }
    foreach ($files as $file) unlink($dir . '/' . $file);
    rmdir($dir);
}

if (($argv[1] ?? '') !== '--child') return;

$path = $argv[2];
$format = pathinfo($path, PATHINFO_EXTENSION);
$points = [];
for ($i = 0; $i < 40; $i++) $points[] = (($i * 7919) % 1000) / 1000;
$chart = (new FastChart\LineChart(2048, 2048))
    ->setSeries([['name' => 's', 'data' => $points]])
    ->setWebpMode(FastChart\Chart::WEBP_LOSSLESS);
$cheap = [];
$full = [];
for ($i = 0; $i < 2; $i++) {
    $chart->setPngCompressionLevel(0);
    $start = fastchartTimeoutClock();
    $chart->renderToFile($path . '.png');
    $cheap[] = fastchartTimeoutClock() - $start;
    unlink($path . '.png');
    $chart->setPngCompressionLevel(9);
    $start = fastchartTimeoutClock();
    $chart->renderToFile($path);
    $full[] = fastchartTimeoutClock() - $start;
    unlink($path);
}

/* File sinks avoid charging large uncompressed smart_str growth to the
 * rasterization estimate under the tracked allocator. Both measurements
 * use PHP's deadline clock, including on Windows. */
$base = max($cheap);
$total = min($full);
if ($total < $base * 1.3 + 0.08) {
    printf("insufficient encoder separation: cheap=%s full=%s\n",
        json_encode($cheap), json_encode($full));
    exit(1);
}
$budget = $base + ($total - $base) * 0.4;
$limit = (int) ceil($budget) + 1;
printf("armed %s cheap=%s full=%s budget=%.4f limit=%d\n",
    $format, json_encode($cheap), json_encode($full), $budget, $limit);
register_shutdown_function(static function (): void {});
ini_set('max_execution_time', (string) $limit);
$stop = fastchartTimeoutClock() + $limit - $budget;
while (fastchartTimeoutClock() < $stop) {}
$chart->renderToFile($path);
echo "returned\n";
