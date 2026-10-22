--TEST--
Deprecation gate cleans up its temporary SVG before returning error status
--SKIPIF--
<?php
if (!function_exists('proc_open')) die('skip proc_open unavailable');
?>
--FILE--
<?php
$dir = sys_get_temp_dir() . '/fc-gate-test-' . bin2hex(random_bytes(8));
mkdir($dir);
$stub = $dir . '/stub.php';
file_put_contents($stub, <<<'STUB'
<?php
namespace FastChart;
class Chart {
    private static int $renders = 0;
    public function __construct(...$args) {}
    public function __call($name, $args) { return $this; }
    public function renderSvg() {
        if (++self::$renders === 2) {
            if (getenv('FC_GATE_TEST') === 'error') {
                throw new \RuntimeException('test render failure');
            }
            if (getenv('FC_GATE_TEST') === 'deprecated') {
                trigger_error('test deprecation', E_USER_DEPRECATED);
            }
        }
        return '<svg/>';
    }
    public function renderToFile($path) { file_put_contents($path, '<svg/>'); }
    public static function svgToPng($svg) { return "\x89PNG"; }
}
foreach (['LineChart', 'AreaChart', 'BarChart', 'PieChart', 'ScatterChart', 'StockChart', 'QrCode'] as $name) {
    class_alias(Chart::class, __NAMESPACE__ . '\\' . $name);
}
require $argv[1];
STUB
);
$gate = dirname(__DIR__) . '/scripts/deprecation-gate.php';
try {
    foreach (['clean' => 0, 'error' => 2, 'deprecated' => 1] as $scenario => $expected) {
        $env = getenv();
        $env['TMPDIR'] = $env['TMP'] = $env['TEMP'] = $dir;
        $env['FC_GATE_TEST'] = $scenario;
        $process = proc_open([PHP_BINARY, '-n', $stub, $gate],
            [0 => ['pipe', 'r'], 1 => ['pipe', 'w'], 2 => ['pipe', 'w']], $pipes, null, $env);
        if (!is_resource($process)) throw new RuntimeException('cannot start PHP');
        fclose($pipes[0]);
        $stdout = stream_get_contents($pipes[1]); fclose($pipes[1]);
        $stderr = stream_get_contents($pipes[2]); fclose($pipes[2]);
        $status = proc_close($process);
        $diagnostic = match ($scenario) {
            'clean' => $stdout === "deprecation-gate clean: no E_DEPRECATED under E_ALL\n" && $stderr === '',
            'error' => $stdout === '' && $stderr === "deprecation-gate ERROR: RuntimeException: test render failure\n",
            'deprecated' => $stdout === '' && str_contains($stderr, 'deprecation-gate DEPRECATED: test deprecation'),
        };
        if ($status !== $expected || !$diagnostic) {
            echo json_encode(['status' => $status, 'stdout' => $stdout, 'stderr' => $stderr]), "\n";
        }
        echo "$scenario status: ", $status === $expected ? 'OK' : 'FAIL', "\n";
        echo "$scenario diagnostic: ", $diagnostic ? 'OK' : 'FAIL', "\n";
        echo "$scenario cleanup: ", !file_exists($dir . '/fc-deprecation-gate.svg') ? 'OK' : 'FAIL', "\n";
        @unlink($dir . '/fc-deprecation-gate.svg');
    }
} finally {
    @unlink($dir . '/fc-deprecation-gate.svg');
    unlink($stub);
    rmdir($dir);
}
?>
--EXPECT--
clean status: OK
clean diagnostic: OK
clean cleanup: OK
error status: OK
error diagnostic: OK
error cleanup: OK
deprecated status: OK
deprecated diagnostic: OK
deprecated cleanup: OK
