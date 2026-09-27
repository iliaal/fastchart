--TEST--
renderToFile() distinguishes registered wrappers from local and drive paths
--EXTENSIONS--
fastchart
--FILE--
<?php
final class FastchartProbeWrapper
{
    public $context;
    public static int $opened = 0;
    public function stream_open(string $path, string $mode): bool
    {
        self::$opened++;
        return true;
    }
    public function stream_write(string $data): int { return strlen($data); }
    public function stream_close(): void {}
}
stream_wrapper_register('fcprobe', FastchartProbeWrapper::class);
$dir = sys_get_temp_dir() . '/fastchart-wrapper-' . getmypid();
mkdir($dir);
$cwd = getcwd();
chdir($dir);
$chart = (new FastChart\LineChart(200, 120))->setSeries([1, 2, 3]);
$symbol = (new FastChart\Code128())->setData('FC-12345')->setSize(400, 100);
function attempt(object $object, string $path): string
{
    try {
        $object->renderToFile($path);
        return 'wrote';
    } catch (ValueError $e) {
        return str_contains($e->getMessage(), 'only supports local filesystem paths')
            ? 'wrapper-refused' : 'value-error';
    } catch (Throwable $e) {
        return 'local-error';
    }
}
try {
    foreach ([$chart, $symbol] as $object) {
        var_dump(attempt($object, 'plain.png') === 'wrote');
        var_dump(file_get_contents('plain.png', false, null, 0, 8) === "\x89PNG\r\n\x1a\n");
        unlink('plain.png');
        foreach (['file://' . $dir . '/f.png', 'FILE://' . $dir . '/g.png',
                  'data://payload.png', 'php://memory/o.png',
                  'http://example.com/o.png', 'fcprobe://o.png'] as $path) {
            var_dump(attempt($object, $path) === 'wrapper-refused');
        }
        $drive = 'C:\\fc-missing-' . getmypid() . '\\o.png';
        var_dump(attempt($object, $drive) !== 'wrapper-refused');
        if (is_file($drive)) unlink($drive);
        var_dump(attempt($object, 'c:/fc-missing-' . getmypid() . '/o.png') !== 'wrapper-refused');
        var_dump(attempt($object, 'fcunknown://o.png') === 'local-error');
        var_dump(attempt($object, $dir . '/missing/http://o.png') === 'local-error');
    }
    var_dump(FastchartProbeWrapper::$opened);
} finally {
    stream_wrapper_unregister('fcprobe');
    chdir($cwd);
    rmdir($dir);
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
int(0)
