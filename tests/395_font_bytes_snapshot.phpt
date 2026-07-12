--TEST--
Font faces retain PHP-stream bytes when the backing pathname changes
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') echo "skip: no system font present\n";
?>
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';
$font = fc_pick_font();
$copy = tempnam(sys_get_temp_dir(), 'fastchart-font-');
copy($font, $copy);

function font_chart(string $font, string $title): FastChart\LineChart
{
    return (new FastChart\LineChart(300, 180))
        ->setFontPath($font)
        ->setTitle($title)
        ->setSeries([1, 2, 3]);
}

$cached = font_chart($copy, 'Warm cache');
$cached->renderSvg();

file_put_contents($copy, 'not a font');
$cached->setTitle('Glyphs XYZ');
$actual = $cached->renderSvg();
$expected = font_chart($font, 'Glyphs XYZ')->renderSvg();

echo $actual === $expected ? "retained\n" : "CHANGED\n";
unlink($copy);
?>
--EXPECT--
retained
