--TEST--
Chart::renderToFile('*.pdf') failure leaves the destination intact
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Mirrors 425_encoder_file_sink.phpt's chart_failure_atomic on the PDF
 * lane: a render that throws must neither truncate the destination nor
 * leave staging temps. Deliberately no --SKIPIF-- so this also executes
 * on pdfio-less lanes (including Windows): there every .pdf render
 * fails closed with "not compiled in" before staging, and the same
 * preserved + no-temps asserts document that path. */

$dir = sys_get_temp_dir() . '/fastchart-pdf-atomic-'
	. bin2hex(random_bytes(6));
mkdir($dir, 0700);

$preserved = $dir . '/keep.pdf';
file_put_contents($preserved, 'old-good-data');
$broken = (new FastChart\LineChart(320, 200))
	->setSeries([1, 2, 3])
	->setYAxisRange(100, null);
try {
	$broken->renderToFile($preserved);
	echo "chart: no throw\n";
} catch (ValueError $e) {
	echo 'chart failure atomic: ',
		str_contains($e->getMessage(), 'resolved min')
		&& file_get_contents($preserved) === 'old-good-data'
		&& glob($preserved . '.fctmp-*') === []
			? "yes\n" : "NO\n";
} catch (Error $e) {
	echo 'chart failure atomic: ',
		str_contains($e->getMessage(), 'not compiled in')
		&& file_get_contents($preserved) === 'old-good-data'
		&& glob($preserved . '.fctmp-*') === []
			? "yes\n" : "NO\n";
}

/* Symbols reject .pdf (raster/svg only) regardless of the pdfio build,
 * so this lane is deterministic everywhere. */
$symPath = $dir . '/symbol.pdf';
file_put_contents($symPath, 'sym-orig');
try {
	(new FastChart\QrCode())->setData('ATOMIC-446')->renderToFile($symPath);
	echo "symbol: no throw\n";
} catch (ValueError $e) {
	echo 'symbol failure atomic: ',
		str_contains($e->getMessage(), 'could not infer format')
		&& file_get_contents($symPath) === 'sym-orig'
		&& glob($symPath . '.fctmp-*') === []
			? "yes\n" : "NO\n";
}

foreach (glob($dir . '/*') as $path) unlink($path);
rmdir($dir);

?>
--EXPECT--
chart failure atomic: yes
symbol failure atomic: yes
