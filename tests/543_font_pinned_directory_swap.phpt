--TEST--
Font loading follows a font directory swapped under an unchanged path
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (PHP_OS_FAMILY !== 'Linux') die("skip Linux pinned-directory test\n");
if (fc_pick_font() === '') die("skip no system font available\n");
?>
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';
$font = fc_pick_font();
$root = __DIR__ . '/543_fontdirs';
@mkdir($root);
file_put_contents("$root/junk.ttf", 'not a font');
copy($font, "$root/font.ttf");

function chart(string $font): FastChart\LineChart
{
    return (new FastChart\LineChart(300, 120))
        ->setSeries([1, 3, 2])
        ->setTitle('Hamburgefonstiv')
        ->setFontPath($font);
}

$good = chart($font)->renderSvg();
$bad = chart("$root/junk.ttf")->renderSvg();

// A deploy that swaps the font directory: the setter pinned the old
// directory, the path now names a new one. Run once without and once
// with an open_basedir that contains both directories.
foreach (['no open_basedir' => false, 'open_basedir' => true] as $label => $narrow) {
    $dir = "$root/" . ($narrow ? 'b' : 'a');
    @mkdir($dir);
    @mkdir("$dir/fonts");
    copy($font, "$dir/fonts/font.ttf");
    $c = chart("$dir/fonts/font.ttf");
    $warm = $c->renderSvg() === $good;

    rename("$dir/fonts", "$dir/fonts.old");
    mkdir("$dir/fonts");
    copy("$root/junk.ttf", "$dir/fonts/font.ttf");
    if ($narrow) ini_set('open_basedir', $root);
    echo "$label: warm ", $warm ? "font" : "OTHER", ", after swap ",
        $c->renderSvg() === $bad ? "new directory" : "OLD DIRECTORY", "\n";

    copy("$root/font.ttf", "$dir/fonts/font.ttf");
    unlink("$dir/fonts.old/font.ttf");
    rmdir("$dir/fonts.old");
    echo "$label: old directory removed, ",
        $c->renderSvg() === $good ? "font" : "FONT LOST", "\n";
}
?>
--CLEAN--
<?php
$root = __DIR__ . '/543_fontdirs';
foreach (['a', 'b'] as $d) {
    @unlink("$root/$d/fonts/font.ttf");
    @unlink("$root/$d/fonts.old/font.ttf");
    @rmdir("$root/$d/fonts");
    @rmdir("$root/$d/fonts.old");
    @rmdir("$root/$d");
}
@unlink("$root/junk.ttf");
@unlink("$root/font.ttf");
@rmdir($root);
?>
--EXPECT--
no open_basedir: warm font, after swap new directory
no open_basedir: old directory removed, font
open_basedir: warm font, after swap new directory
open_basedir: old directory removed, font
