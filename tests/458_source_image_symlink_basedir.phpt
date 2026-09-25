--TEST--
Source-image open_basedir check follows the resolved symlink target
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (!function_exists('symlink')) die("skip symlink support required\n");
?>
--FILE--
<?php

$base = sys_get_temp_dir();
$pid = getmypid();
$inside = $base . '/fc_obd_inside_' . $pid;
$outside = $base . '/fc_obd_outside_' . $pid;
$link = $inside . '/linked.png';
$outsideFile = $outside . '.png';
@mkdir($inside, 0700);
$png = base64_decode(
    'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII='
);
file_put_contents($outsideFile, $png);
symlink($outsideFile, $link);

$chart = (new FastChart\LineChart(120, 80))
    ->setBackgroundImage($link)
    ->setSeries([1, 2, 3]);
ini_set('open_basedir', $inside);
$svg = $chart->renderSvg();
echo str_contains($svg, '<image ') ? "symlink-bypassed\n" : "symlink-refused\n";

@unlink($link);
@unlink($outsideFile);
@rmdir($inside);

?>
--EXPECT--
symlink-refused
