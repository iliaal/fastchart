--TEST--
Deprecation gate isolates temporary render files and cleans up after errors
--SKIPIF--
<?php
if (!function_exists('proc_open')) die('skip proc_open unavailable');
?>
--FILE--
<?php
$dir = sys_get_temp_dir() . '/fc-gate-test-' . bin2hex(random_bytes(8));
mkdir($dir);
$mock = <<<'PHP'
<?php
namespace FastChart;
class Chart {
    public function __construct(...$args) {}
    public function __call($name, $args) { return $this; }
    public function renderSvg() { return '<svg/>'; }
    public function renderToFile($path) {
        file_put_contents(getenv('FC_GATE_PATH'), $path);
        file_put_contents($path, '<svg/>');
        if (getenv('FC_GATE_THROW')) throw new \RuntimeException('render failed');
    }
    public static function svgToPng($svg) { return "\x89PNG"; }
}
class LineChart extends Chart {} class AreaChart extends Chart {}
class BarChart extends Chart {} class PieChart extends Chart {}
class ScatterChart extends Chart {} class StockChart extends Chart {}
class QrCode extends Chart {}
PHP;
file_put_contents($dir . '/mock.php', $mock);
file_put_contents($dir . '/fc-deprecation-gate.svg', 'keep me');
$paths = [];
try {
    foreach (['first', 'second', 'render error', 'allocation error'] as $scenario) {
        $env = getenv();
        $env['FC_GATE_PATH'] = $dir . '/path';
        $env['FC_GATE_THROW'] = $scenario === 'render error' ? '1' : '';
        @unlink($dir . '/path');
        $args = [PHP_BINARY, '-n', '-d', 'display_errors=0', '-d', 'error_reporting=-1',
            '-d', 'sys_temp_dir=' . ($scenario === 'allocation error' ? $dir . '/absent' : $dir),
            '-d', 'auto_prepend_file=' . $dir . '/mock.php',
            dirname(__DIR__) . '/scripts/deprecation-gate.php'];
        $proc = proc_open($args, [0 => ['pipe', 'r'], 1 => ['pipe', 'w'],
            2 => ['pipe', 'w']], $pipes, null, $env);
        if (!is_resource($proc)) throw new RuntimeException('cannot start PHP');
        fclose($pipes[0]);
        $out = stream_get_contents($pipes[1]); fclose($pipes[1]);
        $err = stream_get_contents($pipes[2]); fclose($pipes[2]);
        $status = proc_close($proc);
        $ok = is_file($dir . '/fc-deprecation-gate.svg')
            && file_get_contents($dir . '/fc-deprecation-gate.svg') === 'keep me';
        if ($scenario === 'allocation error') {
            $ok = $ok && $status === 2 && $out === '' && !file_exists($dir . '/path')
                && str_contains($err, 'Cannot create render gate temporary directory');
        } else {
            $path = file_get_contents($dir . '/path');
            $ok = $ok && dirname($path) !== $dir && str_ends_with($path, '/gate.svg')
                && !file_exists($path) && !is_dir(dirname($path));
            $paths[] = $path;
            $ok = $ok && ($scenario === 'render error'
                ? $status === 2 && $out === '' && str_contains($err, 'render failed')
                : $status === 0 && $err === '' && str_contains($out, 'deprecation-gate clean:'));
        }
        echo "$scenario: ", $ok ? 'OK' : 'FAIL', "\n";
    }
    echo 'unique paths: ', count(array_unique($paths)) === 3 ? 'OK' : 'FAIL', "\n";
} finally {
    foreach (glob($dir . '/fc-deprecation-gate-*') as $subdir) {
        foreach (glob($subdir . '/*') as $file) unlink($file);
        rmdir($subdir);
    }
    foreach (glob($dir . '/*') as $file) unlink($file);
    rmdir($dir);
}
?>
--EXPECT--
first: OK
second: OK
render error: OK
allocation error: OK
unique paths: OK
