--TEST--
Decoded icon surfaces use a bounded reuse-value cache
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php

/* Eviction is behavioral: thirteen 1152x1152 icons decode past the 64
 * MiB reuse budget, so the first twelve must be re-decoded when reused
 * below — yet every placement still draws its own colour. A plutosvg
 * refresh that dropped the bounded policy would either blow the budget
 * or mis-draw; both show up in the pixel asserts, not in source text. */

$dir = sys_get_temp_dir() . '/fastchart-icons-' . bin2hex(random_bytes(6));
mkdir($dir, 0700);
$paths = [];
$values = array_fill(0, 32, 50.0);
$chart = (new FastChart\LineChart(960, 360))
	->setPlotRect(20, 20, 939, 339)
	->setYAxisRange(0.0, 100.0);

for ($i = 0; $i < 13; $i++) {
	$path = $dir . '/icon-' . $i . '.png';
	$image = imagecreatetruecolor(1152, 1152);
	$red = ($i * 37) & 0xff;
	$green = ($i * 67) & 0xff;
	$blue = ($i * 97) & 0xff;
	imagefill($image, 0, 0,
		imagecolorallocate($image, $red, $green, $blue));
	imagepng($image, $path, 1);
	$paths[] = $path;
}

$chart->setSeries($values);
for ($i = 0; $i < 12; $i++) {
	$chart->addIconAt((float)$i, 50.0, $paths[$i], 24, 24);
}
$chart->addIconAt(12.0, 50.0, $paths[12], 24, 24);
for ($i = 0; $i < 12; $i++) {
	$chart->addIconAt((float)($i + 13), 50.0, $paths[$i], 24, 24);
}
for ($i = 25; $i < 32; $i++) {
	$chart->addIconAt((float)$i, 50.0, $paths[12], 24, 24);
}
$png = $chart->renderPng();
$decoded = imagecreatefromstring($png);

echo 'over-budget render: ',
	$decoded !== false
	&& imagesx($decoded) === 960
	&& imagesy($decoded) === 360
		? "yes\n" : "NO\n";

$last = 31;
$sampleX = 20 + (int)((($last + 0.5) / count($values))
	* (939 - 20) + 0.5);
$sampleY = 339 - (int)(0.5 * (339 - 20) + 0.5);
$pixel = imagecolorat($decoded, $sampleX, $sampleY);
$expected = [(12 * 37) & 0xff, (12 * 67) & 0xff,
	(12 * 97) & 0xff];
$actual = [($pixel >> 16) & 0xff, ($pixel >> 8) & 0xff,
	$pixel & 0xff];
echo 'hot icon drawn after eviction: ',
	$actual === $expected ? "yes\n" : "NO\n";

foreach ($paths as $path) unlink($path);
rmdir($dir);

?>
--EXPECT--
over-budget render: yes
hot icon drawn after eviction: yes
