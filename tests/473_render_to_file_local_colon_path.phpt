--TEST--
renderToFile() keeps a local path whose directory component contains a colon
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (PHP_OS_FAMILY === 'Windows') echo 'skip colon directory names require POSIX';
?>
--FILE--
<?php
$dir = sys_get_temp_dir() . '/fastchart-colon-' . getmypid();
mkdir($dir . '/x:', 0777, true);
$cwd = getcwd();
chdir($dir);
$chart = (new FastChart\LineChart(200, 120))->setSeries([1, 2, 3]);
$symbol = (new FastChart\Code128())->setData('FC-12345')->setSize(400, 100);
try {
    foreach ([$chart, $symbol] as $index => $object) {
        foreach ([$dir . '/x://', $dir . '/x:/', './x:/'] as $prefix) {
            $path = $prefix . $index . '.png';
            $object->renderToFile($path);
            var_dump(file_get_contents($path, false, null, 0, 8) === "\x89PNG\r\n\x1a\n");
            unlink($path);
        }
    }
} finally {
    chdir($cwd);
    rmdir($dir . '/x:');
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
