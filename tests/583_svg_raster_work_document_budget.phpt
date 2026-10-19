--TEST--
svgToPng: rasterizer coverage work is budgeted per document
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

function zigzag(int $n, string $attrs = ''): string
{
    $d = 'M0 0';
    for ($i = 0; $i < $n; $i++) {
        $d .= ' L' . ($i % 4096) . ' ' . (($i & 1) ? 4096 : 0);
    }
    return '<svg xmlns="http://www.w3.org/2000/svg" width="4096" height="4096">'
        . '<path d="' . $d . '"' . $attrs . '/></svg>';
}

/* 32,000 full-height segments in a 300 KB path: ~870M work units, about
 * 20 s of CPU to rasterize. Refused once the 2^26 budget runs out, as
 * a stroke of the same path is. The refusal itself costs the budget,
 * ~1.5 s in a release build and ~10 s in a loaded -O0 sanitizer build;
 * CPU time, with a bound 3x above that. */
foreach (['fill' => '', 'stroke' => ' fill="none" stroke="#000"'] as $kind => $attrs) {
    $t = cpu_seconds();
    try {
        FastChart\Chart::svgToPng(zigzag(32000, $attrs));
        echo "$kind: rendered\n";
    } catch (ValueError $e) {
        echo "$kind: ", $e->getMessage(), "\n";
    }
    $t = cpu_seconds() - $t;
    echo "$kind cpu: ", ($t < 30.0 ? "bounded" : sprintf("%.1fs", $t)), "\n";
}

?>
--EXPECT--
fill: FastChart\Chart::svgToPng() SVG render work exceeds cap (fill coverage too large)
fill cpu: bounded
stroke: FastChart\Chart::svgToPng() SVG render work exceeds cap (fill coverage too large)
stroke cpu: bounded
