--TEST--
Font loading never serves bytes from a pinned font directory moved outside open_basedir
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (PHP_OS_FAMILY !== 'Linux') die("skip Linux pinned-directory test\n");
if (fc_pick_font() === '') die("skip no system font available\n");
if (!function_exists('symlink')) die("skip symlink unavailable\n");
?>
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';
$font = fc_pick_font();
$root = __DIR__ . '/542_fontdirs';
$allowed = "$root/allowed";
$out = "$root/out";
foreach ([$root, $allowed, $out, "$allowed/d", "$allowed/e"] as $d) @mkdir($d);
copy($font, "$allowed/d/font.ttf");
copy($font, "$allowed/e/font.ttf");
copy($font, "$allowed/link.ttf");
file_put_contents("$root/junk.ttf", 'not a font');

function chart(string $font): FastChart\LineChart
{
    return (new FastChart\LineChart(300, 120))
        ->setSeries([1, 3, 2])
        ->setTitle('Hamburgefonstiv')
        ->setFontPath($font);
}

$good = chart($font)->renderSvg();
$bad = chart("$root/junk.ttf")->renderSvg();

// Each setter pins the font's parent directory and the first render
// loads the face from it.
$moved = chart("$allowed/d/font.ttf");
$relocated = chart("$allowed/e/font.ttf");
$swapped = chart("$allowed/link.ttf");
// Bound before the narrowing, rendered after it: the reference output
// for a font that open_basedir rejects.
$outside = chart("$root/junk.ttf");
echo 'warm: ', $moved->renderSvg() === $good
    && $relocated->renderSvg() === $good ? "font\n" : "OTHER\n";

// Move the authorized directories outside the future open_basedir and
// put new files at the same paths: unparseable bytes for d, the same
// font for e. Replace link.ttf with a symlink to a font outside.
rename("$allowed/d", "$out/d");
mkdir("$allowed/d");
file_put_contents("$allowed/d/font.ttf", 'not a font');
rename("$allowed/e", "$out/e");
mkdir("$allowed/e");
copy($font, "$allowed/e/font.ttf");
unlink("$allowed/link.ttf");
symlink("$out/e/font.ttf", "$allowed/link.ttf");

ini_set('open_basedir', $allowed);

echo 'moved directory: ', $moved->renderSvg() === $bad
    ? "reads the in-base file\n" : "SERVES THE OUTSIDE FONT\n";
echo 'relocated directory: ', $relocated->renderSvg() === $good
    ? "reads the in-base font\n" : "FONT LOST\n";
$rejected = $outside->renderSvg();
echo 'rejection reference is fontless: ',
    $rejected !== $good ? "yes\n" : "NO\n";
echo 'symlink outside: ', $swapped->renderSvg() === $rejected
    ? "rejected\n" : "SERVES THE OUTSIDE FONT\n";
?>
--CLEAN--
<?php
$root = __DIR__ . '/542_fontdirs';
foreach (['allowed/d/font.ttf', 'allowed/e/font.ttf', 'allowed/link.ttf',
          'out/d/font.ttf', 'out/e/font.ttf', 'junk.ttf'] as $f) {
    @unlink("$root/$f");
}
foreach (['allowed/d', 'allowed/e', 'allowed', 'out/d', 'out/e', 'out', ''] as $d) {
    @rmdir("$root/$d");
}
?>
--EXPECT--
warm: font
moved directory: reads the in-base file
relocated directory: reads the in-base font
rejection reference is fontless: yes
symlink outside: rejected
