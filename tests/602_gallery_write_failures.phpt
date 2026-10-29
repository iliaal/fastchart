--TEST--
Gallery generators report output write failures instead of success
--SKIPIF--
<?php
if (!function_exists('proc_open')) die('skip proc_open unavailable');
?>
--FILE--
<?php
$root = dirname(__DIR__);
$dir = sys_get_temp_dir() . '/fc-gallery-write-' . bin2hex(random_bytes(8));
mkdir($dir);
mkdir($dir . '/scripts');
mkdir($dir . '/docs');
$html = "<!doctype html><html><body>gallery</body></html>\n";
try {
    foreach (['build-v1-gallery.php' => 'docs/v1-gallery.html',
              'build-svg-compare.php' => 'compare-svg-png.html'] as $name => $output) {
        // Exercise the committed output stage without rendering every chart.
        $source = file_get_contents($root . '/scripts/' . $name);
        $start = strpos($source, '$out = __DIR__');
        if ($start === false) throw new RuntimeException('missing output stage');
        $script = $dir . '/scripts/' . $name;
        file_put_contents($script, '<?php $html = ' . var_export($html, true)
            . '; $ncharts = 1; $total_svg_kb = $total_png_kb = 0;'
            . substr($source, $start));
        $path = $dir . '/' . $output;
        foreach (['success', 'blocked'] as $scenario) {
            if ($scenario === 'blocked') mkdir($path);
            $process = proc_open([PHP_BINARY, '-n', '-d', 'display_errors=0', $script],
                [0 => ['pipe', 'r'], 1 => ['pipe', 'w'], 2 => ['pipe', 'w']], $pipes);
            if (!is_resource($process)) throw new RuntimeException('cannot start PHP');
            fclose($pipes[0]);
            $stdout = stream_get_contents($pipes[1]); fclose($pipes[1]);
            $stderr = stream_get_contents($pipes[2]); fclose($pipes[2]);
            $status = proc_close($process);
            if ($scenario === 'success') {
                $ok = $status === 0 && $stderr === '' && str_starts_with($stdout, 'wrote ')
                    && file_get_contents($path) === $html;
                unlink($path);
            } else {
                $ok = $status !== 0 && $stdout === '' && is_dir($path)
                    && str_contains($stderr, 'Failed to write gallery: ');
                rmdir($path);
            }
            echo "$name $scenario: ", $ok ? 'OK' : 'FAIL', "\n";
        }
        unlink($script);
    }
} finally {
    foreach (glob($dir . '/scripts/*') as $file) unlink($file);
    rmdir($dir . '/scripts');
    rmdir($dir . '/docs');
    rmdir($dir);
}
?>
--EXPECT--
build-v1-gallery.php success: OK
build-v1-gallery.php blocked: OK
build-svg-compare.php success: OK
build-svg-compare.php blocked: OK
