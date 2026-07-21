--TEST--
Chart and Symbol SVG string, file, fragment, text-mode, and failure parity
--EXTENSIONS--
fastchart
--FILE--
<?php

function check(string $label, bool $condition): void
{
	echo $label, ': ', $condition ? "yes\n" : "NO\n";
}

function full_document(string $svg): bool
{
	return str_starts_with($svg, '<?xml ')
		&& str_contains($svg, '<svg xmlns="http://www.w3.org/2000/svg"')
		&& str_contains($svg, 'xmlns:xlink="http://www.w3.org/1999/xlink"')
		&& str_ends_with($svg, "</svg>\n");
}

function fragment_group(string $svg, string $class): bool
{
	return str_starts_with($svg, '<g class="' . $class . '">')
		&& str_ends_with($svg, "</g>\n")
		&& !str_contains($svg, '<?xml')
		&& !str_contains($svg, '<svg')
		&& !str_contains($svg, 'xmlns=');
}

function file_parity(object $renderer, string $path): array
{
	$svg = $renderer->renderSvg();
	$written = $renderer->renderToFile($path);
	$disk = file_get_contents($path);
	return [$svg, $written === strlen($svg) && $disk === $svg];
}

function failure_preserves(
	object $renderer,
	string $path,
	callable $repair
): bool {
	$sentinel = "existing destination\n";
	file_put_contents($path, $sentinel);
	$before = glob(dirname($path) . '/*');
	$threw = false;
	try {
		$renderer->renderToFile($path);
	} catch (Throwable $e) {
		$threw = true;
	}
	$after = glob(dirname($path) . '/*');
	$preserved = file_get_contents($path) === $sentinel;
	$repair($renderer);
	return $threw && $preserved && $before === $after
		&& full_document($renderer->renderSvg());
}

$dir = sys_get_temp_dir() . '/fc-svg-parity-' . getmypid();
mkdir($dir, 0700);
$paths = [];
$path = function (string $name) use ($dir, &$paths): string {
	$file = $dir . '/' . $name . '.svg';
	$paths[] = $file;
	return $file;
};
register_shutdown_function(function () use (&$paths, $dir): void {
	foreach ($paths as $file) {
		@unlink($file);
	}
	@rmdir($dir);
});

$chart = (new FastChart\BarChart(320, 220))
	->setSeries([3, 7, 5])
	->setCategoryLabels(['Alpha', 'Beta', 'Gamma'])
	->setTitle('Path text')
	->setGradientFill(0x2266aa, 0x88ccee);
[$chartSvg, $chartParity] = file_parity($chart, $path('chart-paths'));
check('chart paths file parity', $chartParity && full_document($chartSvg));
check('chart paths text', !str_contains($chartSvg, '<text')
	&& str_contains($chartSvg, '<path'));

$chart->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
	->setTitle('Native text');
[$nativeSvg, $nativeParity] = file_parity($chart, $path('chart-native'));
check('chart native file parity', $nativeParity && full_document($nativeSvg));
check('chart native text', str_contains($nativeSvg, '<text'));

$gradientFragment = $chart->drawSvgFragment('grad_');
check('chart fragment group', fragment_group($gradientFragment, 'fastchart'));
check('gradient fragment ids', str_contains($gradientFragment, 'id="grad_fcg1"')
	&& str_contains($gradientFragment, 'url(#grad_fcg1)')
	&& !preg_match('/url\(#fcg\d/', $gradientFragment));

$clipFragment = (new FastChart\Pictogram(300, 150))
	->setTotal(10)->setValue(2.5)->setIconCount(10)
	->drawSvgFragment('clip_');
check('clip fragment ids', str_contains($clipFragment, 'id="clip_fcc1"')
	&& str_contains($clipFragment, 'url(#clip_fcc1)')
	&& !preg_match('/url\(#fcc\d/', $clipFragment));

$code128 = (new FastChart\Code128())
	->setData('FASTCHART-12345')->setSize(400, 100)->setShowText(true);
[$codeSvg, $codeParity] = file_parity($code128, $path('code128-paths'));
check('code128 paths file parity', $codeParity && full_document($codeSvg));
check('code128 paths text', !str_contains($codeSvg, '<text')
	&& str_contains($codeSvg, '<path'));
check('code128 fragment group', fragment_group(
	$code128->drawSvgFragment('code_'), 'fastchart-symbol'));

$code128->setSvgTextMode(FastChart\Symbol::SVG_TEXT_NATIVE);
[$codeNativeSvg, $codeNativeParity] = file_parity(
	$code128, $path('code128-native'));
check('code128 native file parity', $codeNativeParity
	&& full_document($codeNativeSvg));
check('code128 native text', str_contains($codeNativeSvg, '<text'));

$qr = (new FastChart\QrCode())
	->setData('https://example.com/svg-parity')
	->setSize(180, 180)
	->setEcc(FastChart\QrCode::ECC_Q);
[$qrSvg, $qrParity] = file_parity($qr, $path('qrcode'));
check('qrcode file parity', $qrParity && full_document($qrSvg));
check('qrcode fragment group', fragment_group(
	$qr->drawSvgFragment('qr_'), 'fastchart-symbol'));

check('chart failure cleanup', failure_preserves(
	(new FastChart\Pictogram(200, 100))->setValue(1),
	$path('chart-failure'),
	fn (FastChart\Pictogram $c) => $c->setTotal(10)
));
check('code128 failure cleanup', failure_preserves(
	new FastChart\Code128(),
	$path('code128-failure'),
	fn (FastChart\Code128 $c) => $c->setData('RECOVERED')
));
check('qrcode failure cleanup', failure_preserves(
	new FastChart\QrCode(),
	$path('qrcode-failure'),
	fn (FastChart\QrCode $q) => $q->setData('RECOVERED')
));

?>
--EXPECT--
chart paths file parity: yes
chart paths text: yes
chart native file parity: yes
chart native text: yes
chart fragment group: yes
gradient fragment ids: yes
clip fragment ids: yes
code128 paths file parity: yes
code128 paths text: yes
code128 fragment group: yes
code128 native file parity: yes
code128 native text: yes
qrcode file parity: yes
qrcode fragment group: yes
chart failure cleanup: yes
code128 failure cleanup: yes
qrcode failure cleanup: yes
