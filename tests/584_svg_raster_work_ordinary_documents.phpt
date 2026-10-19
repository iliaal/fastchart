--TEST--
svgToPng: a heavy ordinary document fits the raster work budget
--EXTENSIONS--
fastchart
--FILE--
<?php

function series(int $s): array
{
    $p = [];
    for ($i = 0; $i < 2048; $i++) {
        $p[] = (($i * 7919 + $s * 104729) % 1000) / 1000;
    }
    return ['name' => "s$s", 'data' => $p];
}

function line_chart(int $w, int $h, int $first, int $count): FastChart\LineChart
{
    return (new FastChart\LineChart($w, $h))
        ->setMarkerStyle(FastChart\Chart::MARKER_NONE)
        ->setSeries(array_map('series', range($first, $first + $count - 1)));
}

/* Eight stitched 1024x512 line charts of eight 2048-point series, the
 * largest ordinary document the budget was sized for: ~1.01G of the 1.2G
 * work units. Rendered at 2560 px (~1.35G units) it is refused. */
$g = '';
for ($k = 0; $k < 8; $k++) {
    $g .= sprintf('<g transform="translate(%d %d)">%s</g>',
        ($k % 2) * 1024, intdiv($k, 2) * 512,
        line_chart(1024, 512, $k, 8)->drawSvgFragment("c$k"));
}
$png = FastChart\Chart::svgToPng(
    '<svg xmlns="http://www.w3.org/2000/svg" width="2048" height="2048">'
    . $g . '</svg>');
$info = getimagesizefromstring($png);
echo "stitched charts: {$info[0]}x{$info[1]}\n";
?>
--EXPECT--
stitched charts: 2048x2048
