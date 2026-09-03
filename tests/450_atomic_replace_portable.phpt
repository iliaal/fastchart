--TEST--
renderToFile replace-existing is atomic and leaves no staging temps (portable)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Portable atomic-fallback coverage: no POSIX fixtures (/proc, strace,
 * symlinks, proc_open, junctions), so this executes on the Windows lane
 * too, where the NtSetInformationFile install path otherwise has no
 * dedicated test. Overwriting an existing destination must swap bytes
 * atomically, a failed render must preserve the destination, and no
 * .fctmp staging residue may remain either way. */
$dir = sys_get_temp_dir() . '/fastchart-replace-'
	. bin2hex(random_bytes(6));
mkdir($dir, 0700);

$chart = (new FastChart\LineChart(360, 220))->setSeries([1, 4, 2, 5]);
$path = $dir . '/chart.png';

$first = $chart->renderPng();
$written = $chart->renderToFile($path);
echo 'initial write exact: ',
	$written === strlen($first)
	&& file_get_contents($path) === $first ? "yes\n" : "NO\n";

$chart->setTitle('second render');
$second = $chart->renderPng();
$written2 = $chart->renderToFile($path);
echo 'replace exact: ',
	$written2 === strlen($second)
	&& file_get_contents($path) === $second
	&& $second !== $first ? "yes\n" : "NO\n";

$broken = (new FastChart\LineChart(320, 200))
	->setSeries([1, 2, 3])
	->setYAxisRange(100, null);
try {
	$broken->renderToFile($path);
	echo "failure: no throw\n";
} catch (ValueError $e) {
	echo 'failure preserves destination: ',
		file_get_contents($path) === $second ? "yes\n" : "NO\n";
}

echo 'no staging residue: ',
	glob($path . '.fctmp-*') === []
	&& glob($dir . '/*.fctmp-*') === [] ? "yes\n" : "NO\n";

foreach (glob($dir . '/*') as $file) unlink($file);
rmdir($dir);

?>
--EXPECT--
initial write exact: yes
replace exact: yes
failure preserves destination: yes
no staging residue: yes
