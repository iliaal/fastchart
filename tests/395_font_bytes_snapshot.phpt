--TEST--
Font faces keep their byte snapshot within a render and reload the rewritten file on the next render
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
file_put_contents($junk, 'not a font');

final class FontRewriteWrapper
{
    public static string $font = '';
    public static string $png = '';
    public $context;
    private int $pos = 0;

    public function stream_open($path, $mode, $options, &$opened): bool
    {
        file_put_contents(self::$font, 'not a font');
        $this->pos = 0;
        return true;
    }

    public function stream_read($n): string
    {
        $chunk = substr(self::$png, $this->pos, $n);
        $this->pos += strlen($chunk);
        return $chunk;
    }

    public function stream_eof(): bool
    {
        return $this->pos >= strlen(self::$png);
    }

    public function stream_stat(): array
    {
        return ['size' => strlen(self::$png)];
    }

    public function url_stat($path, $flags): array
    {
        return ['size' => strlen(self::$png), 'mode' => 0100644];
    }
}

function font_chart(string $font, string $title): FastChart\LineChart
{
    return (new FastChart\LineChart(300, 180))
        ->setFontPath($font)
        ->setTitle($title)
        ->setSeries([1, 2, 3]);
}

FontRewriteWrapper::$font = $copy;
FontRewriteWrapper::$png = (new FastChart\LineChart(40, 30))
    ->setSeries([1, 2])->renderPng();
stream_wrapper_register('fcrewrite', 'FontRewriteWrapper');

// Layout loads the face, the background wrapper then rewrites the font
// file in place, and the title glyphs are emitted afterwards. The face
// owns a private copy of the bytes, so the rewrite cannot change (or
// fault) the glyphs of the render in progress.
copy($font, $copy);
$snapshot = font_chart($copy, 'Glyphs XYZ')
    ->setBackgroundImage('fcrewrite://bg')->renderSvg();
echo 'rewritten during render: ',
    file_get_contents($copy) === 'not a font' ? "yes\n" : "NO\n";
copy($font, $copy);
FontRewriteWrapper::$font = $junk;
$original = font_chart($font, 'Glyphs XYZ')
    ->setBackgroundImage('fcrewrite://bg')->renderSvg();
echo $snapshot === $original ? "retained within render\n" : "CHANGED WITHIN RENDER\n";

// The next render reopens the path and reads the rewritten file.
$cached = font_chart($copy, 'Warm cache');
$cached->renderSvg();
file_put_contents($copy, 'not a font');
$cached->setTitle('Glyphs XYZ');
$actual = $cached->renderSvg();
$rewritten = font_chart($junk, 'Glyphs XYZ')->renderSvg();
$stale = font_chart($font, 'Glyphs XYZ')->renderSvg();
echo $actual === $rewritten ? "reloaded on next render\n"
    : ($actual === $stale ? "STALE\n" : "CHANGED\n");
unlink($copy);
unlink($junk);
?>
--EXPECT--
rewritten during render: yes
retained within render
reloaded on next render
