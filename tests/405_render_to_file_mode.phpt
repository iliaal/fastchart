--TEST--
Atomic renderToFile replacement preserves modes and new files honor umask
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (PHP_OS_FAMILY === 'Windows') {
	die('skip POSIX file modes are not available on Windows');
}
?>
--FILE--
<?php

$dir = sys_get_temp_dir() . '/fc-mode-' . getmypid();
@mkdir($dir, 0700);
$chart = (new FastChart\LineChart(160, 100))->setSeries([1, 2]);
$oldUmask = umask(0027);

$new = "$dir/new.svg";
$chart->renderToFile($new);
clearstatcache(true, $new);
printf("new: %04o\n", fileperms($new) & 0777);

chmod($new, 0600);
$chart->renderToFile($new);
clearstatcache(true, $new);
printf("chart replace: %04o\n", fileperms($new) & 0777);

$symbol = "$dir/symbol.svg";
file_put_contents($symbol, 'old');
chmod($symbol, 0644);
(new FastChart\Code128(200, 80))->setData('1234')->renderToFile($symbol);
clearstatcache(true, $symbol);
printf("symbol replace: %04o\n", fileperms($symbol) & 0777);

$dropbox = "$dir/dropbox";
mkdir($dropbox, 0333);
$dropboxPath = "$dropbox/out.svg";
$chart->renderToFile($dropboxPath);
echo 'search-only parent: ',
	is_file($dropboxPath) ? "yes\n" : "NO\n";

umask($oldUmask);
@unlink($dropboxPath);
chmod($dropbox, 0700);
@rmdir($dropbox);
@unlink($new);
@unlink($symbol);
@rmdir($dir);
?>
--EXPECT--
new: 0640
chart replace: 0600
symbol replace: 0644
search-only parent: yes
