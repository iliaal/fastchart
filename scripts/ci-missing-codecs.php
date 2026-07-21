<?php

declare(strict_types=1);

$temporaryFiles = [];

register_shutdown_function(static function () use (&$temporaryFiles): void {
	foreach ($temporaryFiles as $path) {
		if (is_file($path)) {
			unlink($path);
		}
	}
});

function fail(string $message): never
{
	fwrite(STDERR, "MISSING-CODEC FAIL: $message\n");
	exit(1);
}

function expectError(string $label, callable $operation,
	string $expectedMessage): void
{
	try {
		$operation();
	} catch (Error $error) {
		if ($error->getMessage() === $expectedMessage) {
			printf("OK %s\n", $label);
			return;
		}
		fail(sprintf('%s: expected error %s, got %s', $label,
			var_export($expectedMessage, true),
			var_export($error->getMessage(), true)));
	}

	fail("$label: operation did not throw");
}

function assertSvg(string $label, string $svg): void
{
	if (!str_starts_with($svg, '<?xml ') || !str_contains($svg, '</svg>')) {
		fail("$label: SVG output is incomplete");
	}
	printf("OK %s\n", $label);
}

function temporaryPath(string $extension): string
{
	global $temporaryFiles;

	$path = sys_get_temp_dir() . '/fastchart-missing-codec-' .
		bin2hex(random_bytes(12)) . ".{$extension}";
	$temporaryFiles[] = $path;
	return $path;
}

ob_start();
phpinfo(INFO_MODULES);
$moduleInfo = ob_get_clean();
if (!is_string($moduleInfo)) {
	fail('phpinfo() did not return module information');
}
foreach (['libpng', 'libjpeg', 'libwebp'] as $library) {
	$pattern = '/^' . preg_quote($library, '/') .
		'\s+=>\s+\(not compiled in\)$/m';
	if (preg_match($pattern, $moduleInfo) !== 1) {
		fail("phpinfo() does not report $library as not compiled in");
	}
}
echo "OK phpinfo codec rows\n";

$chart = (new FastChart\LineChart(160, 100))->setSeries([1, 3, 2]);
$symbol = (new FastChart\QrCode())->setData('missing-codec-smoke')
	->setSize(120, 120);
$chartSvg = $chart->renderSvg();
assertSvg('Chart renderSvg', $chartSvg);
assertSvg('Symbol renderSvg', $symbol->renderSvg());

$codecs = [
	'Png' => ['png', 'libpng'],
	'Jpeg' => ['jpg', 'libjpeg-turbo'],
	'Webp' => ['webp', 'libwebp'],
];

foreach ($codecs as $suffix => [$extension, $library]) {
	$method = "render{$suffix}";
	expectError("Chart::$method", static fn() => $chart->$method(),
		"FastChart: $library support not compiled in " .
		'(configure could not find the library at build time)');
	expectError("Symbol::$method", static fn() => $symbol->$method(),
		"FastChart\\Symbol: $library support not compiled in " .
		'(configure could not find the library at build time)');

	$chartPath = temporaryPath($extension);
	expectError("Chart::renderToFile .$extension",
		static fn() => $chart->renderToFile($chartPath),
		"FastChart\\Chart::renderToFile(): $library support not compiled in " .
		'(configure could not find the library at build time)');
	if (file_exists($chartPath)) {
		fail("Chart::renderToFile .$extension created a destination file");
	}

	$symbolPath = temporaryPath($extension);
	expectError("Symbol::renderToFile .$extension",
		static fn() => $symbol->renderToFile($symbolPath),
		"FastChart\\Symbol::renderToFile(): $library support not compiled in " .
		'(configure could not find the library at build time)');
	if (file_exists($symbolPath)) {
		fail("Symbol::renderToFile .$extension created a destination file");
	}
}

$staticCodecs = [
	'svgToPng' => 'libpng',
	'svgToJpeg' => 'libjpeg',
	'svgToWebp' => 'libwebp',
];
foreach ($staticCodecs as $method => $library) {
	expectError("Chart::$method", static fn() => FastChart\Chart::$method($chartSvg),
		"FastChart\\Chart::$method(): $library is not compiled in");
}

echo "missing-codec smoke passed\n";
