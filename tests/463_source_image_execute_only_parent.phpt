--TEST--
Source-image loader traverses execute-only parent directories
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (DIRECTORY_SEPARATOR === '\\' || !function_exists('chmod')) {
    die("skip POSIX mode test unsupported\n");
}
?>
--FILE--
<?php

$dir = sys_get_temp_dir() . '/fc_exec_parent_' . getmypid();
$file = $dir . '/image.png';
@mkdir($dir, 0700);
file_put_contents($file, base64_decode(
    'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII='
));
chmod($file, 0444);
chmod($dir, 0111);
$svg = (new FastChart\LineChart(120, 80))
    ->setBackgroundImage($file)
    ->setSeries([1, 2, 3])
    ->renderSvg();
echo str_contains($svg, '<image ') ? "execute-only-parent-ok\n" : "execute-only-parent-missing\n";
chmod($dir, 0700);
@unlink($file);
@rmdir($dir);

?>
--EXPECT--
execute-only-parent-ok
