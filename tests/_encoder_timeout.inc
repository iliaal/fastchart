<?php
function fastchartTimeoutUsesWallClock(): bool
{
    static $wall = null;
    if ($wall === null) {
        ob_start();
        phpinfo(INFO_GENERAL);
        $info = ob_get_clean();
        /* Apple Silicon's CPU accounting is unsuitable for PHP's timer;
         * Zend's per-thread timers also measure elapsed time. */
        $wall = PHP_OS_FAMILY === 'Windows'
            || (PHP_OS_FAMILY === 'Darwin'
                && in_array(php_uname('m'), ['arm64', 'aarch64'], true))
            || str_contains($info, 'Zend Max Execution Timers => enabled');
    }
    return $wall;
}

function fastchartTimeoutClock(): float
{
    if (fastchartTimeoutUsesWallClock()) return hrtime(true) / 1e9;
    $r = getrusage();
    return $r['ru_utime.tv_sec'] + $r['ru_utime.tv_usec'] / 1e6
        + $r['ru_stime.tv_sec'] + $r['ru_stime.tv_usec'] / 1e6;
}

function fastchartEncoderTimeout(string $format, bool $replace = false): void
{
    $dir = sys_get_temp_dir() . '/fastchart-encoder-' . $format . '-' . getmypid();
    mkdir($dir);
    $path = $dir . '/output.' . $format;
    $command = escapeshellarg(PHP_BINARY) . ' ' . getenv('TEST_PHP_ARGS')
        . ' -d memory_limit=512M -d hard_timeout=0 '
        . escapeshellarg(__FILE__) . ' --child ' . escapeshellarg($path)
        . ($replace ? ' replace' : '');
    $output = [];
    exec($command . ' 2>&1', $output, $status);
    $text = implode("\n", $output);
    $files = array_values(array_diff(scandir($dir), ['.', '..']));
    $expected = $replace ? ['output.' . $format] : [];
    $preserved = !$replace || (is_file($path)
        && file_get_contents($path) === 'prior destination');
    $ok = str_contains($text, 'armed ')
        && str_contains($text, 'Maximum execution time')
        && !str_contains($text, 'returned')
        && $status === 255 && $files === $expected && $preserved;
    if ($ok) {
        echo $replace ? "timeout preserves prior file\n" : "timeout before file commit\n";
    } else {
        printf("status=%d files=%s preserved=%s\n%s\n", $status,
            json_encode($files), $preserved ? 'yes' : 'no', $text);
    }
    foreach ($files as $file) unlink($dir . '/' . $file);
    rmdir($dir);
}

if (($argv[1] ?? '') !== '--child') return;

$path = $argv[2];
$format = pathinfo($path, PATHINFO_EXTENSION);
$points = [];
for ($i = 0; $i < 40; $i++) $points[] = (($i * 7919) % 1000) / 1000;
$measurements = [];
foreach ([4096, 6144, 8000] as $size) {
    $chart = (new FastChart\LineChart($size, $size))
        ->setSeries([['name' => 's', 'data' => $points]])
        ->setWebpMode(FastChart\Chart::WEBP_LOSSLESS);
    /* Cold render and codec setup must not be compared with a warm encode. */
    $chart->setPngCompressionLevel(1)->renderToFile($path . '.png');
    unlink($path . '.png');
    $chart->setPngCompressionLevel(9)->renderToFile($path);
    unlink($path);
    $cheap = [];
    $full = [];
    for ($i = 0; $i < 2; $i++) {
        $chart->setPngCompressionLevel(1);
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
    $base = max($cheap);
    $total = min($full);
    $measurements[$size] = ['cheap' => $cheap, 'full' => $full];
    /* Leave a useful cancellation window beyond platform timer coalescing. */
    if ($total >= $base * 1.3 + 0.08 && $total - $base >= 0.5) break;
}

/* File sinks avoid tracked-allocator smart_str growth. Compressing the
 * cheap pass also avoids charging a large raw file write to its Windows
 * wall-clock measurement. Both passes use PHP's deadline clock. */
if ($total < $base * 1.3 + 0.08 || $total - $base < 0.5) {
    printf("insufficient encoder separation: %s\n", json_encode($measurements));
    exit(1);
}
$budget = $base + ($total - $base) * 0.4;
$limit = (int) ceil($budget) + 1;
if (($argv[3] ?? '') === 'replace') file_put_contents($path, 'prior destination');
printf("armed %s size=%d cheap=%s full=%s budget=%.4f limit=%d\n",
    $format, $size, json_encode($cheap), json_encode($full), $budget, $limit);
register_shutdown_function(static function (): void {});
ini_set('max_execution_time', (string) $limit);
$stop = fastchartTimeoutClock() + $limit - $budget;
while (fastchartTimeoutClock() < $stop) {}
$chart->renderToFile($path);
echo "returned\n";
