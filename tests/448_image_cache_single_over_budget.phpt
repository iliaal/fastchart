--TEST--
A single over-budget image still renders: the cache is a memory control, not a rendering one
--EXTENSIONS--
fastchart
--INI--
asan.detect_leaks=0
fastchart.max_image_cache_bytes=512
--FILE--
<?php

/* Companion to 443 (budget shared across icons) and 426 (eviction with
 * reuse). tests/__icon.png is 16x16, so it decodes to 1024 RGBA bytes:
 * twice the 512-byte budget here, and the only image in the render, so
 * it can never be retained. The render must still succeed with the icon
 * drawn — over budget means "decode every time", never "drop the
 * image". No gd needed: the with/without byte-difference proves the
 * icon contributes pixels, and getimagesizefromstring() is core. */
echo 'configured: ', ini_get('fastchart.max_image_cache_bytes'), "\n";

$icon = __DIR__ . '/__icon.png';

$with = (new FastChart\LineChart(640, 240))
	->setPlotRect(20, 20, 619, 219)
	->setYAxisRange(0.0, 100.0)
	->setSeries(array_fill(0, 8, 50.0))
	->addIconAt(3.5, 50.0, $icon, 24, 24)
	->renderPng();
$without = (new FastChart\LineChart(640, 240))
	->setPlotRect(20, 20, 619, 219)
	->setYAxisRange(0.0, 100.0)
	->setSeries(array_fill(0, 8, 50.0))
	->renderPng();

$info = getimagesizefromstring($with);
echo 'render over budget: ',
	$info !== false && $info[0] === 640 && $info[1] === 240
		? "yes\n" : "NO\n";
echo 'over-budget icon drawn: ',
	$with !== $without ? "yes\n" : "NO\n";

$chart = (new FastChart\LineChart(640, 240))
	->setPlotRect(20, 20, 619, 219)
	->setYAxisRange(0.0, 100.0)
	->setSeries(array_fill(0, 8, 50.0))
	->addIconAt(3.5, 50.0, $icon, 24, 24);
echo 'sha stable across renders: ',
	hash('sha256', $chart->renderPng()) === hash('sha256', $with)
		? "yes\n" : "NO\n";

?>
--EXPECT--
configured: 512
render over budget: yes
over-budget icon drawn: yes
sha stable across renders: yes
