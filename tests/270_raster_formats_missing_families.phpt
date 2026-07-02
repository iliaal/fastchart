--TEST--
Raster round-trip for the families test 131 misses: Bullet/Pareto/Calendar/Sunburst/Sankey/Marimekko/Vector
--EXTENSIONS--
fastchart
gd
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die('skip no system font available');
?>
--FILE--
<?php
/* Test 131 covers 31 chart families; these seven shipped later (or
 * were skipped) and had SVG-only coverage — a raster-path break in
 * any of them (rasterize, un-premultiply, encoder) went unseen. */

require __DIR__ . '/_font_candidates.inc';
$font = fc_pick_font();

$families = [
    'BulletChart' => fn() => (new FastChart\BulletChart(400, 80))
        ->setRange(0, 100)
        ->setBands([
            ['from' => 0,  'to' => 60],
            ['from' => 60, 'to' => 85],
            ['from' => 85, 'to' => 100],
        ])
        ->setValue(72)->setTarget(80),
    'ParetoChart' => fn() => (new FastChart\ParetoChart(400, 300))
        ->setBars([
            ['label' => 'a', 'value' => 40],
            ['label' => 'b', 'value' => 30],
            ['label' => 'c', 'value' => 10],
        ]),
    'CalendarHeatmap' => fn() => (new FastChart\CalendarHeatmap(600, 140))
        ->setData(['2026-01-05' => 3, '2026-02-14' => 9, '2026-03-15' => 5])
        ->setColorRamp(0xEEFFEE, 0x004400),
    'SunburstChart' => fn() => (new FastChart\SunburstChart(300, 300))
        ->setHierarchy([
            'label' => 'root',
            'children' => [
                ['label' => 'A', 'value' => 10],
                ['label' => 'B', 'value' => 20],
            ],
        ]),
    'SankeyChart' => fn() => (new FastChart\SankeyChart(400, 250))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([
            ['from' => 0, 'to' => 2, 'value' => 5],
            ['from' => 1, 'to' => 2, 'value' => 3],
        ]),
    'MarimekkoChart' => fn() => (new FastChart\MarimekkoChart(400, 300))
        ->setColumns([
            ['label' => 'Q1', 'segments' => [
                ['label' => 'x', 'value' => 30],
                ['label' => 'y', 'value' => 20],
            ]],
            ['label' => 'Q2', 'segments' => [
                ['label' => 'x', 'value' => 40],
                ['label' => 'y', 'value' => 10],
            ]],
        ]),
    'VectorChart' => fn() => (new FastChart\VectorChart(300, 300))
        ->setVectors([
            ['x' => 0, 'y' => 0, 'dx' => 1, 'dy' => 1],
            ['x' => 1, 'y' => 0, 'dx' => -1, 'dy' => 1],
            ['x' => 0, 'y' => 1, 'dx' => 1, 'dy' => -1],
        ]),
];

$fail = 0;
foreach ($families as $name => $build) {
    $c = $build()->setFontPath($font);

    $png  = $c->renderPng();
    $jpg  = $c->renderJpeg();
    $webp = $c->renderWebp();
    $svg  = $c->renderSvg();

    $row_ok = true;
    if (substr($png, 0, 8) !== "\x89PNG\r\n\x1a\n" || strlen($png) < 200) {
        echo "FAIL $name PNG: ", strlen($png), " bytes\n";
        $row_ok = false;
    } else {
        /* Decode round-trip so a corrupt-but-magic-prefixed stream fails. */
        $im = imagecreatefromstring($png);
        if (!$im || imagesx($im) < 100) {
            echo "FAIL $name PNG decode\n";
            $row_ok = false;
        }
    }
    if (substr($jpg, 0, 3) !== "\xFF\xD8\xFF" || strlen($jpg) < 200) {
        echo "FAIL $name JPEG: ", strlen($jpg), " bytes\n";
        $row_ok = false;
    }
    if (substr($webp, 0, 4) !== 'RIFF' || substr($webp, 8, 4) !== 'WEBP'
        || strlen($webp) < 100) {
        echo "FAIL $name WebP: ", strlen($webp), " bytes\n";
        $row_ok = false;
    }
    if (substr($svg, 0, 6) !== '<?xml ' || !str_contains($svg, '<svg ')) {
        echo "FAIL $name SVG: ", strlen($svg), " bytes\n";
        $row_ok = false;
    }
    if ($row_ok) {
        echo "$name: PNG/JPEG/WebP/SVG ok\n";
    } else {
        $fail++;
    }
}
echo $fail === 0 ? "ALL OK\n" : "FAILED $fail/" . count($families) . "\n";
?>
--EXPECT--
BulletChart: PNG/JPEG/WebP/SVG ok
ParetoChart: PNG/JPEG/WebP/SVG ok
CalendarHeatmap: PNG/JPEG/WebP/SVG ok
SunburstChart: PNG/JPEG/WebP/SVG ok
SankeyChart: PNG/JPEG/WebP/SVG ok
MarimekkoChart: PNG/JPEG/WebP/SVG ok
VectorChart: PNG/JPEG/WebP/SVG ok
ALL OK
