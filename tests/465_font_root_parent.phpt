--TEST--
Root open_basedir permits an ordinary regular font path
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die("skip no system font present\n");
?>
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';

ini_set('open_basedir', '/');
$font = fc_pick_font();
if ($font === '') die("skip no system font present\n");
$chart = new FastChart\LineChart(120, 80);
try {
    $chart->setFontPath($font)->setTitle('root-parent')->setSeries([1, 2, 3]);
    echo strlen($chart->renderSvg()) > 100 ? "root-open-ok\n" : "root-open-empty\n";
} catch (Error $e) {
    echo "root-open-threw\n";
}
?>
--EXPECT--
root-open-ok
