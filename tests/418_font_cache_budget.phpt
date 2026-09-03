--TEST--
FreeType face cache enforces an aggregate backing-byte budget
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
    // font while its fixture is still present; the unlinks below then
    // exercise the render-time fallback path (evicted vs retained).
    $oldest_chart = cache_chart($paths[0]);
    $retained_chart = cache_chart($paths[1]);
    unlink($paths[0]);
    $oldest = $oldest_chart->renderSvg();
    echo $oldest !== $expected[0] ? "evicted oldest\n" : "OLDEST RETAINED\n";

    unlink($paths[1]);
    $retained = $retained_chart->renderSvg();
    echo $retained === $expected[1]
        ? "retained within budget\n" : "RETAINED FACE LOST\n";
} finally {
    foreach ($paths as $path) {
        if (is_file($path)) unlink($path);
    }
}
?>
--EXPECT--
evicted oldest
retained within budget
