--TEST--
Heatmap PNG title color does not leak into cell labels or the plot
--EXTENSIONS--
fastchart
gd
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die('skip no system font available');
?>
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';
$chart = (new FastChart\Heatmap(400, 300))->setGrid([[0, 1]])
    ->setFontPath(fc_pick_font())->setTitleFont(null, 20)->setTitle('Heading')
    ->setTextColor(0x123456)->setColorRamp(0xFFFFFF, 0x000000)->setShowValues(true);
$original = $chart->renderPng();
$a = imagecreatefromstring($original);
$b = imagecreatefromstring($chart->setTitleColor(0xAB12CD)->renderPng());
$changed = $magenta = 0;
$plotSame = true;
for ($y = 0; $y < 300; $y++) {
    for ($x = 0; $x < 400; $x++) {
        $before = imagecolorat($a, $x, $y);
        $after = imagecolorat($b, $x, $y);
        if ($y < 50) {
            if ($before !== $after) $changed++;
            if (($after & 0xFFFFFF) === 0xAB12CD) $magenta++;
        } elseif ($before !== $after) {
            $plotSame = false;
        }
    }
}
echo 'title recolored: ', $changed > 20 && $magenta > 20 ? 'OK' : 'FAIL', "\n";
echo 'plot and labels unchanged: ', $plotSame ? 'OK' : 'FAIL', "\n";
echo 'reset exact bytes: ', $chart->setTitleColor(-1)->renderPng() === $original ? 'OK' : 'FAIL', "\n";
?>
--EXPECT--
title recolored: OK
plot and labels unchanged: OK
reset exact bytes: OK
