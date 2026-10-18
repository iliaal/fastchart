--TEST--
Font faces reload from PHP-stream bytes when the backing file is rewritten in place
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
$junk = tempnam(sys_get_temp_dir(), 'fastchart-font-');
copy($font, $copy);
file_put_contents($junk, 'not a font');

function font_chart(string $font, string $title): FastChart\LineChart
{
    return (new FastChart\LineChart(300, 180))
        ->setFontPath($font)
        ->setTitle($title)
        ->setSeries([1, 2, 3]);
}

$cached = font_chart($copy, 'Warm cache');
$cached->renderSvg();

// The cached face is a private byte copy, so rewriting the file cannot
// change pages under it; the next render re-reads the rewritten file.
file_put_contents($copy, 'not a font');
$cached->setTitle('Glyphs XYZ');
$actual = $cached->renderSvg();
$rewritten = font_chart($junk, 'Glyphs XYZ')->renderSvg();
$stale = font_chart($font, 'Glyphs XYZ')->renderSvg();

echo $actual === $rewritten ? "reloaded\n"
    : ($actual === $stale ? "STALE\n" : "CHANGED\n");
unlink($copy);
unlink($junk);
?>
--EXPECT--
reloaded
