--TEST--
svgToPng: tall edges are not re-walked over every scanline for every raster band
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

/* One fill of 500 near-vertical full-height edges. The cell pool splits
 * the canvas into about a thousand bands, and each band used to walk
 * every scanline of every edge (bands x edges x height, ~30 s under
 * ASan). Clipping the walk to the band makes it linear in the cells
 * the band actually holds. CPU time, not wall time, with a bound about
 * 10x above the fixed cost on a slow sanitizer build. */
$d = 'M0 0';
for ($i = 0; $i < 500; $i++) {
    $d .= ' L' . $i . ' ' . (($i & 1) ? 2048 : 0);
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
