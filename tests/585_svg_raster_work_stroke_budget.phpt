--TEST--
svgToPng: rasterizer coverage work of a stroke is budgeted per document
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
    $d = 'M0 0';
    for ($i = 0; $i < $n; $i++) {
        $d .= ' L' . ($i % 4096) . ' ' . (($i & 1) ? 4096 : 0);
    }
    return '<svg xmlns="http://www.w3.org/2000/svg" width="4096" height="4096">'
        . '<path d="' . $d . '"' . $attrs . '/></svg>';
}

/* The 8,000-segment path of 583, stroked: stroke outlines rasterize
 * through the same coverage walk and share the budget (over 4G units
 * unbudgeted); under ASan 4f3caa5 does not finish in 900 s. The refusal
 * spends the whole budget: 3.9 s release on an idle host, 9 s on a
 * loaded one, 14-42 s for an -O0 ASan+UBSan module. The CPU-time bound
 * sits 3x above the slowest of those and 7x below 900 s. */
$t = cpu_seconds();
try {
    FastChart\Chart::svgToPng(zigzag(8000, ' fill="none" stroke="#000"'));
    echo "rendered\n";
} catch (ValueError $e) {
    echo $e->getMessage(), "\n";
}
$t = cpu_seconds() - $t;
echo "cpu: ", ($t < 120.0 ? "bounded" : sprintf("%.1fs", $t)), "\n";
?>
--EXPECT--
FastChart\Chart::svgToPng() SVG render work exceeds cap (fill coverage too large)
cpu: bounded
