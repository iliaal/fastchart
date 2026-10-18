--TEST--
Font face cache revalidates the file behind an unchanged path
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die("skip no system font available\n");
?>
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';
$font = fc_pick_font();
$dir = __DIR__ . '/541_fontdir';
@mkdir($dir);
$path = "$dir/font.ttf";
$junk = "$dir/junk.ttf";
copy($font, $path);
file_put_contents($junk, 'not a font');

function chart(string $font): FastChart\LineChart
{
    return (new FastChart\LineChart(300, 120))
        ->setSeries([1, 3, 2])
        ->setTitle('Hamburgefonstiv')
        ->setFontPath($font);
}

$good = chart($font)->renderSvg();
$bad = chart($junk)->renderSvg();
echo 'fixtures differ: ', $good !== $bad ? "yes\n" : "NO\n";

$c = chart($path);
echo 'warm: ', $c->renderSvg() === $good ? "font\n" : "OTHER\n";

copy($junk, "$path.tmp");
rename("$path.tmp", $path);
clearstatcache();
echo 'after rename over: ', $c->renderSvg() === $bad ? "replacement\n" : "STALE\n";

unlink($path);
copy($font, $path);
echo 'after recreate: ', $c->renderSvg() === $good ? "font\n" : "OTHER\n";
?>
--CLEAN--
<?php
$dir = __DIR__ . '/541_fontdir';
@unlink("$dir/font.ttf");
@unlink("$dir/font.ttf.tmp");
@unlink("$dir/junk.ttf");
@rmdir($dir);
?>
--EXPECT--
fixtures differ: yes
warm: font
after rename over: replacement
after recreate: font
