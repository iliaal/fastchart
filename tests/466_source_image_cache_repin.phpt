--TEST--
Source-image loader re-pins a valid path after pinned-parent cache eviction
--EXTENSIONS--
fastchart
--FILE--
<?php

$dir = sys_get_temp_dir() . '/fc_pin_eviction_' . getmypid();
@mkdir($dir, 0700);
$png = base64_decode(
    'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII='
);
$first = $dir . '/first.png';
file_put_contents($first, $png);
ini_set('open_basedir', $dir);
$chart = (new FastChart\LineChart(120, 80))->setBackgroundImage($first);
$filler = new FastChart\LineChart(120, 80);
for ($i = 1; $i < 65; $i++) {
    $path = $dir . '/image' . $i . '.png';
    file_put_contents($path, $png);
    $filler->setBackgroundImage($path);
}
$svg = $chart->setSeries([1, 2, 3])->renderSvg();
echo str_contains($svg, '<image ') ? "evicted-path-repinned\n" : "evicted-path-missing\n";
@unlink($first);
for ($i = 1; $i < 65; $i++) @unlink($dir . '/image' . $i . '.png');
@rmdir($dir);

?>
--EXPECT--
evicted-path-repinned
