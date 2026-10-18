--TEST--
FreeType face cache evicts past its byte budget within a render and never serves a deleted font across renders
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

final class FontDeleteWrapper
{
    public static array $victims = [];
    public static string $png = '';
    public $context;
    private int $pos = 0;

    public function stream_open($path, $mode, $options, &$opened): bool
    {
        foreach (self::$victims as $victim) {
            if (is_file($victim)) unlink($victim);
        }
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

function fixture(string $source, int $mib): string
{
    $path = tempnam(sys_get_temp_dir(), 'fastchart-font-budget-');
    if ($path === false || !copy($source, $path)) {
        throw new RuntimeException('unable to create font fixture');
    }
    $stream = fopen($path, 'ab');
    if ($stream === false || !ftruncate($stream, $mib * 1024 * 1024)) {
        throw new RuntimeException('unable to size font fixture');
    }
    fclose($stream);
    return $path;
}

function cache_chart(string $font): FastChart\LineChart
{
    return (new FastChart\LineChart(240, 140))
        ->setFontPath($font)
        ->setTitle('Cache budget')
        ->setSeries([1, 3, 2]);
}

function two_font_chart(string $title_font, string $axis_font): FastChart\LineChart
{
    return (new FastChart\LineChart(300, 180))
        ->setFontPath($axis_font)
        ->setAxisFont($axis_font)
        ->setLabelFont($axis_font)
        ->setTitleFont($title_font)
        ->setTitle('Cache budget')
        ->setSeries([1, 3, 2])
        ->setBackgroundImage('fcdelete://bg');
}

$source = fc_pick_font();
$paths = [];
FontDeleteWrapper::$png = (new FastChart\LineChart(40, 30))
    ->setSeries([1, 2])->renderPng();
stream_wrapper_register('fcdelete', 'FontDeleteWrapper');
try {
    // Within one render a validated face is reused without reopening,
    // so a face evicted to admit another is the only one that must be
    // reread. Layout loads the title and axis faces, the background
    // wrapper deletes both files, then the glyphs are emitted. Against
    // the 16 MiB budget, 12 + 5 MiB cannot both stay resident; 10 + 5
    // MiB can.
    foreach ([[12, 5, 'over budget'], [10, 5, 'within budget']]
             as [$big, $small, $label]) {
        $title = $paths[] = fixture($source, $big);
        $axis = $paths[] = fixture($source, $small);
        FontDeleteWrapper::$victims = [];
        $full = two_font_chart($title, $axis)->renderSvg();
        $chart = two_font_chart($title, $axis);
        FontDeleteWrapper::$victims = [$title, $axis];
        $after = $chart->renderSvg();
        FontDeleteWrapper::$victims = [];
        echo "$label: files deleted mid-render: ",
            !is_file($title) && !is_file($axis) ? "yes\n" : "NO\n";
        echo "$label: ", $after === $full
            ? "both faces stayed resident\n" : "a face was evicted\n";
    }

    $fonts = [];
    for ($i = 0; $i < 4; $i++) {
        $fonts[] = $paths[] = fixture($source, 5);
    }
    $expected = [];
    foreach ($fonts as $font) {
        $expected[] = cache_chart($font)->renderSvg();
    }

    // Setter fail-fast rejects paths that do not exist, so bind each
    // font while its fixture is still present. Across renders a face is
    // served only after its file reopens, whether its slot was evicted
    // (oldest) or is still resident (newest).
    $oldest_chart = cache_chart($fonts[0]);
    $newest_chart = cache_chart($fonts[3]);
    unlink($fonts[0]);
    echo $oldest_chart->renderSvg() !== $expected[0]
        ? "deleted oldest not served\n" : "OLDEST SERVED\n";
    unlink($fonts[3]);
    echo $newest_chart->renderSvg() !== $expected[3]
        ? "deleted newest not served\n" : "NEWEST SERVED\n";

    // The evicted and reloaded faces still render their own glyphs.
    echo cache_chart($fonts[1])->renderSvg() === $expected[1]
        && cache_chart($fonts[2])->renderSvg() === $expected[2]
        ? "present fonts reload\n" : "PRESENT FONT LOST\n";
} finally {
    foreach ($paths as $path) {
        if (is_file($path)) unlink($path);
    }
}
?>
--EXPECT--
over budget: files deleted mid-render: yes
over budget: a face was evicted
within budget: files deleted mid-render: yes
within budget: both faces stayed resident
deleted oldest not served
deleted newest not served
present fonts reload
