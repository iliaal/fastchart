--TEST--
svgToPng: an outline with many short edges does not re-decompose once per band
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

/* One path of 150,000 small triangles. Every raster band decomposes the
 * whole outline, and with a fixed 32 KB cell pool the band count grew
 * with the cell count, so the cost was edges x cells (~26 s under
 * ASan). The pool now starts proportional to the outline. Bound as in
 * 580. */
mt_srand(7);
$d = '';
for ($i = 0; $i < 150000; $i++) {
    $d .= 'M' . mt_rand(0, 2040) . ' ' . mt_rand(0, 2040) . 'l3 5l3 -5z';
}
$svg = '<svg xmlns="http://www.w3.org/2000/svg" width="2048" height="2048">'
    . '<path d="' . $d . '"/></svg>';

$t = cpu_seconds();
$png = FastChart\Chart::svgToPng($svg);
$t = cpu_seconds() - $t;

$info = getimagesizefromstring($png);
echo "size: {$info[0]}x{$info[1]}\n";
echo "cpu: ", ($t < 10.0 ? "bounded" : sprintf("%.1fs", $t)), "\n";
?>
--EXPECT--
size: 2048x2048
cpu: bounded
