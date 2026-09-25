--TEST--
Root-parent pinned font path opens when the fixture can be created at /
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '' || !is_writable('/')) {
    die("skip writable root fixture unavailable\n");
}
?>
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';
$rootFile = '/fc_fastchart_root_' . getmypid() . '.ttf';
if (!@copy(fc_pick_font(), $rootFile)) die("skip root fixture copy failed\n");
ini_set('open_basedir', '/');
$svg = (new FastChart\LineChart(120, 80))
    ->setFontPath($rootFile)
    ->setTitle('root-parent')
    ->setSeries([1, 2, 3])
    ->renderSvg();
@unlink($rootFile);
echo strlen($svg) > 100 ? "root-parent-pin-ok\n" : "root-parent-pin-empty\n";
?>
--EXPECT--
root-parent-pin-ok
