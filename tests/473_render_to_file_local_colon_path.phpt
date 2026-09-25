--TEST--
renderToFile() keeps a local path whose directory component contains a colon
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fastchart_path_is_wrapper() scanned the whole destination for "://",
 * so the legal POSIX path "<dir>/x://out.png" -- the plain path
 * "<dir>/x:/out.png" -- was refused as a stream wrapper before the
 * parent-directory pinning ran. Only a leading, registered scheme is a
 * wrapper, which is the same rule PHP's own
 * php_stream_locate_url_wrapper() applies. */

final class FastchartProbeWrapper
{
    public $context;
    public static int $opened = 0;

    public function stream_open(string $path, string $mode): bool
    {
        self::$opened++;
        return true;
    }

    public function stream_write(string $data): int
    {
        return strlen($data);
    }

    public function stream_close(): void {}
}

stream_wrapper_register('fcprobe', FastchartProbeWrapper::class);

$dir = sys_get_temp_dir() . '/fastchart-colon-' . getmypid();
mkdir($dir . '/x:', 0777, true);
chdir($dir);

$chart = (new FastChart\LineChart(200, 120))->setSeries([1, 2, 3]);
$symbol = (new FastChart\Code128())->setData('FC-12345')->setSize(400, 100);

/* 'wrote', the wrapper refusal, or any other local-filesystem error. */
function attempt(object $obj, string $path): string
{
    try {
        $obj->renderToFile($path);
        return 'wrote';
    } catch (ValueError $e) {
        return str_contains($e->getMessage(), 'only supports local filesystem paths')
            ? 'wrapper-refused' : 'value-error';
    } catch (Throwable $e) {
        return 'local-error';
    }
}

$png = "\x89PNG\r\n\x1a\n";

/* A colon directory component is an ordinary local path. */
var_dump(attempt($chart, $dir . '/x://out.png'));
var_dump(file_get_contents($dir . '/x:/out.png', false, null, 0, 8) === $png);
var_dump(attempt($chart, $dir . '/x:/out2.png'));
var_dump(file_get_contents($dir . '/x:/out2.png', false, null, 0, 8) === $png);
var_dump(attempt($symbol, $dir . '/x://code128.png'));
var_dump(file_get_contents($dir . '/x:/code128.png', false, null, 0, 8) === $png);
/* Same spelling, relative to the working directory. */
var_dump(attempt($chart, './x:/relative.png'));
var_dump(is_file($dir . '/x:/relative.png'));

/* Ordinary local paths are untouched. */
var_dump(attempt($chart, 'plain.png'));
var_dump(file_get_contents($dir . '/plain.png', false, null, 0, 8) === $png);

/* Drive spellings are never wrapper schemes. 'C:\...' holds no marker
 * at all, and 'c:/' is a one-character prefix, below PHP's two. On
 * Windows both reach the filesystem (or fail there); on POSIX the
 * backslash spelling is one legal filename inside $dir. */
$drive = PHP_OS_FAMILY === 'Windows' ? 'C:\\fcprobe\\o.png' : 'C:\\fc-drive\\o.png';
$rc = attempt($chart, $drive);
if (PHP_OS_FAMILY === 'Windows') {
    var_dump($rc !== 'wrapper-refused');
} else {
    var_dump($rc);
    var_dump(is_file($dir . '/C:\\fc-drive\\o.png'));
}
var_dump(attempt($chart, 'c:/fcprobe.png') !== 'wrapper-refused');

/* Registered schemes stay refused, whatever their spelling. */
var_dump(attempt($chart, 'file://' . $dir . '/f.png'));
var_dump(attempt($chart, 'FILE://' . $dir . '/g.png'));
var_dump(attempt($chart, 'data://payload.png'));
var_dump(attempt($chart, 'php://memory/o.png'));
var_dump(attempt($chart, 'http://example.com/o.png'));
var_dump(attempt($chart, 'fcprobe://o.png'));
var_dump(FastchartProbeWrapper::$opened);

/* An unregistered scheme is a local path, exactly as PHP treats it:
 * the parent directory does not exist, and no wrapper is consulted. */
var_dump(attempt($chart, 'fcunknown://o.png'));

/* Nor can a later component smuggle a wrapper past the check. */
var_dump(attempt($chart, $dir . '/plain/http://o.png'));

/* Cleanup: every probe file is optional, since a probe may legitimately
 * have failed before creating it. */
$remove = static function (string $path): void {
    if (is_file($path)) {
        unlink($path);
    }
};
$remove($dir . '/C:\\fc-drive\\o.png');
$remove($dir . '/plain.png');
foreach (glob($dir . '/x:/*') ?: [] as $f) { $remove($f); }
@rmdir($dir . '/x:');
@rmdir($dir);

?>
--EXPECT--
string(5) "wrote"
bool(true)
string(5) "wrote"
bool(true)
string(5) "wrote"
bool(true)
string(5) "wrote"
bool(true)
string(5) "wrote"
bool(true)
string(5) "wrote"
bool(true)
bool(true)
string(15) "wrapper-refused"
string(15) "wrapper-refused"
string(15) "wrapper-refused"
string(15) "wrapper-refused"
string(15) "wrapper-refused"
string(15) "wrapper-refused"
int(0)
string(11) "local-error"
string(11) "local-error"
