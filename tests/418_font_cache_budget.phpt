--TEST--
FreeType face cache evicts large faces and never serves a deleted font file
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

function cache_chart(string $font): FastChart\LineChart
{
    return (new FastChart\LineChart(240, 140))
        ->setFontPath($font)
        ->setTitle('Cache budget')
        ->setSeries([1, 3, 2]);
}

$source = fc_pick_font();
$paths = [];
try {
    for ($i = 0; $i < 4; $i++) {
        $path = tempnam(sys_get_temp_dir(), 'fastchart-font-budget-');
        if ($path === false || !copy($source, $path)) {
            throw new RuntimeException('unable to create font fixture');
        }
        $stream = fopen($path, 'ab');
        if ($stream === false || !ftruncate($stream, 5 * 1024 * 1024)) {
            throw new RuntimeException('unable to size font fixture');
        }
        fclose($stream);
        $paths[] = $path;
    }

    $expected = [];
    foreach ($paths as $path) {
        $expected[] = cache_chart($path)->renderSvg();
    }

    // Setter fail-fast rejects paths that do not exist, so bind each
    // font while its fixture is still present. Four 5 MiB faces exceed
    // the 16 MiB backing budget, so the loads above evicted cold slots.
    // A face is served only after its file reopens, whether its slot
    // was evicted (oldest) or is still resident (newest).
    $oldest_chart = cache_chart($paths[0]);
    $newest_chart = cache_chart($paths[3]);
    unlink($paths[0]);
    $oldest = $oldest_chart->renderSvg();
    echo $oldest !== $expected[0] ? "deleted oldest not served\n" : "OLDEST SERVED\n";

    unlink($paths[3]);
    $newest = $newest_chart->renderSvg();
    echo $newest !== $expected[3]
        ? "deleted newest not served\n" : "NEWEST SERVED\n";

    // The evicted and reloaded faces still render their own glyphs.
    echo cache_chart($paths[1])->renderSvg() === $expected[1]
        && cache_chart($paths[2])->renderSvg() === $expected[2]
        ? "present fonts reload\n" : "PRESENT FONT LOST\n";
} finally {
    foreach ($paths as $path) {
        if (is_file($path)) unlink($path);
    }
}
?>
--EXPECT--
deleted oldest not served
deleted newest not served
present fonts reload
