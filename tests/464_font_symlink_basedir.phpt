--TEST--
setFontPath pins and rejects a symlink target outside open_basedir
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
$font = fc_pick_font();
if ($font === '' || !function_exists('symlink')) {
    die("skip font and symlink support required\n");
}
?>
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';
$font = fc_pick_font();
$base = sys_get_temp_dir();
$pid = getmypid();
$inside = $base . '/fc_font_in_' . $pid;
$link = $inside . '/font.ttf';
@mkdir($inside, 0700);
symlink($font, $link);
ini_set('open_basedir', $inside);
try {
    @(new FastChart\LineChart(120, 80))->setFontPath($link);
    echo "font-symlink-accepted\n";
} catch (Error $e) {
    echo "font-symlink-rejected\n";
}
@unlink($link);
@rmdir($inside);
?>
--EXPECT--
font-symlink-rejected
