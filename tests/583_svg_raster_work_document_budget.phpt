--TEST--
svgToPng: rasterizer coverage work of a fill is budgeted per document
--EXTENSIONS--
fastchart
--FILE--
<?php

function cpu_seconds(): float
{
    $r = getrusage();
    return $r['ru_utime.tv_sec'] + $r['ru_utime.tv_usec'] / 1e6
        + $r['ru_stime.tv_sec'] + $r['ru_stime.tv_usec'] / 1e6;
}

function zigzag(int $n, string $attrs): string
{
    $d = ['M0 0'];
    for ($i = 0; $i < $n; $i++) {
        $d[] = 'L' . ($i % 4096) . ' ' . (($i & 1) ? 4096 : 0);
    }
    return '<svg xmlns="http://www.w3.org/2000/svg" width="4096" height="4096">'
        . '<path d="' . implode(' ', $d) . '"' . $attrs . '/></svg>';
}

/* 8,000 full-height segments in one 74 KB filled path need ~5.3G work
 * units; under ASan 4f3caa5 does not finish in 900 s. The refusal spends
 * the whole 2.2G-unit budget: 2.9 s release on an idle host, 31 s for an
 * -O0 ASan+UBSan module on a loaded one. The CPU-time bound sits 3.9x
 * above the slower of those and 7x below 900 s. */
$t = cpu_seconds();
try {
    FastChart\Chart::svgToPng(zigzag(8000, ''));
    echo "rendered\n";
} catch (ValueError $e) {
    echo $e->getMessage(), "\n";
}
$t = cpu_seconds() - $t;
echo "cpu: ", ($t < 120.0 ? "bounded" : sprintf("%.1fs", $t)), "\n";

/* 2,048 one-pixel stripes drawn right to left put each new cell at the
 * head of its row list, so no list walk is charged; the cells, sweep
 * visits and spans themselves are. Eight such paths (~4.2G units) used
 * to fit the budget with room to spare, and eighteen took 13.6 s
 * release. The refusal takes 3.0 s release on an idle host and 39 s for
 * the -O0 module on a loaded one, 3x under the same bound. */
$stripes = [];
for ($x = 4094; $x >= 0; $x -= 2) {
    $stripes[] = 'M' . $x . ' 0h1V4096h-1z';
}
$stripes = '<path fill="#369" d="' . implode('', $stripes) . '"/>';
$t = cpu_seconds();
try {
    FastChart\Chart::svgToPng(
        '<svg xmlns="http://www.w3.org/2000/svg" width="4096" height="4096">'
        . str_repeat($stripes, 8) . '</svg>');
    echo "stripes: rendered\n";
} catch (ValueError $e) {
    echo "stripes: ", $e->getMessage(), "\n";
}
$t = cpu_seconds() - $t;
echo "cpu: ", ($t < 120.0 ? "bounded" : sprintf("%.1fs", $t)), "\n";
?>
--EXPECT--
FastChart\Chart::svgToPng() SVG render work exceeds cap (fill coverage too large)
cpu: bounded
stripes: FastChart\Chart::svgToPng() SVG render work exceeds cap (fill coverage too large)
cpu: bounded
