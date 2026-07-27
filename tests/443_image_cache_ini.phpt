--TEST--
fastchart.max_image_cache_bytes lowers the decoded-image budget without changing output
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
fastchart.max_image_cache_bytes=1048576
--FILE--
<?php

/* The INI is PHP_INI_SYSTEM: a script must not be able to raise the
 * ceiling back up after the operator lowered it. */
echo 'configured: ', ini_get('fastchart.max_image_cache_bytes'), "\n";
echo 'ini_set refused: ',
	@ini_set('fastchart.max_image_cache_bytes', '67108864') === false
		? "yes\n" : "NO\n";

/* Four 512x512 icons decode to 1 MiB each, so at a 1 MiB budget at most
 * one fits and every repeat placement pays a fresh decode. A budget is a
 * memory control, never a rendering one: the pixels must come out
 * identical to a run with room for all four. */
$dir = sys_get_temp_dir() . '/fastchart-imgini-' . bin2hex(random_bytes(6));
mkdir($dir, 0700);
$paths = [];
for ($i = 0; $i < 4; $i++) {
	$path = $dir . '/icon-' . $i . '.png';
	$image = imagecreatetruecolor(512, 512);
	imagefill($image, 0, 0, imagecolorallocate($image,
		($i * 61) & 0xff, ($i * 89) & 0xff, ($i * 113) & 0xff));
	imagepng($image, $path, 1);
	imagedestroy($image);
	$paths[] = $path;
}

$chart = (new FastChart\LineChart(640, 240))
	->setPlotRect(20, 20, 619, 219)
	->setYAxisRange(0.0, 100.0)
	->setSeries(array_fill(0, 8, 50.0));
for ($i = 0; $i < 8; $i++) {
	$chart->addIconAt((float)$i, 50.0, $paths[$i % 4], 24, 24);
}
$png = $chart->renderPng();
$decoded = imagecreatefromstring($png);

echo 'render under lowered budget: ',
	$decoded !== false && imagesx($decoded) === 640
	&& imagesy($decoded) === 240 ? "yes\n" : "NO\n";

/* Sample the last placement, which under a 1 MiB budget is guaranteed to
 * have been decoded after at least one eviction. */
$sampleX = 20 + (int)(((7 + 0.5) / 8) * (619 - 20) + 0.5);
$sampleY = 219 - (int)(0.5 * (219 - 20) + 0.5);
$pixel = imagecolorat($decoded, $sampleX, $sampleY);
$expected = [(3 * 61) & 0xff, (3 * 89) & 0xff, (3 * 113) & 0xff];
$actual = [($pixel >> 16) & 0xff, ($pixel >> 8) & 0xff, $pixel & 0xff];
echo 'evicted icon still drawn: ',
	$actual === $expected ? "yes\n" : "NO " . implode(',', $actual) . "\n";

echo 'sha stable across renders: ',
	hash('sha256', $png) === hash('sha256', $chart->renderPng())
		? "yes\n" : "NO\n";

foreach ($paths as $path) unlink($path);
rmdir($dir);

?>
--EXPECT--
configured: 1048576
ini_set refused: yes
render under lowered budget: yes
evicted icon still drawn: yes
sha stable across renders: yes
