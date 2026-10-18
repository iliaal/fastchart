--TEST--
Font face cache does not serve a relative font path resolved under another cwd and open_basedir
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
$root = __DIR__ . '/540_fontdirs';
@mkdir($root);
@mkdir("$root/a");
@mkdir("$root/b");
copy(fc_pick_font(), "$root/a/font.ttf");
file_put_contents("$root/b/font.ttf", 'not a font');

function chart(int $mode = FastChart\Chart::SVG_TEXT_PATHS): FastChart\LineChart
{
    return (new FastChart\LineChart(300, 120))
        ->setSeries([1, 3, 2])
        ->setTitle('Hamburgefonstiv')
        ->setSvgTextMode($mode);
}

function family(string $svg): string
{
    preg_match('/font-family="([^"]*)"/', $svg, $m);
    return $m[1] ?? '';
}

function pdf_body(FastChart\Chart $c): ?string
{
    try {
        $pdf = $c->renderPdf();
    } catch (Error $e) {
        return null;
    }
    return preg_replace([
        '~/ID\[<[0-9A-Fa-f]+><[0-9A-Fa-f]+>\]~',
        '~\(D:[0-9Z+\'-]+\)~',
        '~\d{4}-\d\d-\d\dT\d\d:\d\d:\d\dZ~',
    ], '', $pdf);
}

$native = FastChart\Chart::SVG_TEXT_NATIVE;

chdir("$root/a");
$a = chart()->setFontPath('font.ttf')->renderSvg();
$a_family = family(chart($native)->setFontPath('font.ttf')->renderSvg());

chdir("$root/b");
ini_set('open_basedir', "$root/b");

$b_rel = chart()->setFontPath('font.ttf')->renderSvg();
$b_abs = chart()->setFontPath("$root/b/font.ttf")->renderSvg();
echo 'chart font from cwd a reused: ', $b_rel === $a ? "YES\n" : "no\n";
echo 'chart font matches own file: ', $b_rel === $b_abs ? "yes\n" : "NO\n";

$b_family = family(chart($native)->setFontPath('font.ttf')->renderSvg());
$b_abs_family = family(chart($native)
    ->setFontPath("$root/b/font.ttf")->renderSvg());
echo 'native family from cwd a reused: ',
    $b_family === $a_family ? "YES\n" : "no\n";
echo 'native family matches own file: ',
    $b_family === $b_abs_family ? "yes\n" : "NO\n";

// Axis labels fall back to the auto-detected system font, which the
// narrowed open_basedir excludes, so only the own-file comparison is
// meaningful for a title-only font.
$b_title = chart()->setTitleFont('font.ttf')->renderSvg();
$b_title_abs = chart()->setTitleFont("$root/b/font.ttf")->renderSvg();
echo 'title font matches own file: ',
    $b_title === $b_title_abs ? "yes\n" : "NO\n";

$b_pdf = pdf_body(chart()->setFontPath('font.ttf'));
$b_pdf_abs = pdf_body(chart()->setFontPath("$root/b/font.ttf"));
echo 'pdf font matches own file: ',
    $b_pdf === $b_pdf_abs ? "yes\n" : "NO\n";
?>
--CLEAN--
<?php
$root = __DIR__ . '/540_fontdirs';
@unlink("$root/a/font.ttf");
@unlink("$root/b/font.ttf");
@rmdir("$root/a");
@rmdir("$root/b");
@rmdir($root);
?>
--EXPECT--
chart font from cwd a reused: no
chart font matches own file: yes
native family from cwd a reused: no
native family matches own file: yes
title font matches own file: yes
pdf font matches own file: yes
