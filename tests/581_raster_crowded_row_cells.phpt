--TEST--
svgToPng: a scanline crossed by many edges does not cost edges squared
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

/* 40,000 thin triangles crossing all 64 rows, emitted alternately from
 * the left and right ends so a new cell rarely lands next to the last
 * one. Each row then holds ~80,000 cell records over 4,096 columns, and
 * the rasterizer used to walk the row's sorted cell list on every
 * record (~27 s under ASan). Bound as in 580. */
/* Pieces are joined once: appending to one growing string costs a copy
 * per append without the Zend allocator (USE_ZEND_ALLOC=0 in CI). */
$d = [];
for ($j = 0; $j < 40000; $j++) {
    $k = ($j >> 1) % 4095;
    $x = ($j & 1) ? 4094 - $k : $k;
    $d[] = 'M' . $x . ' 0L' . $x . '.5 64L' . ($x + 1) . ' 0Z';
}
$d = implode('', $d);
$svg = '<svg xmlns="http://www.w3.org/2000/svg" width="4096" height="64">'
    . '<path d="' . $d . '"/></svg>';

$t = cpu_seconds();
$png = FastChart\Chart::svgToPng($svg);
$t = cpu_seconds() - $t;

$info = getimagesizefromstring($png);
echo "size: {$info[0]}x{$info[1]}\n";
echo "cpu: ", ($t < 10.0 ? "bounded" : sprintf("%.1fs", $t)), "\n";
?>
--EXPECT--
size: 4096x64
cpu: bounded
