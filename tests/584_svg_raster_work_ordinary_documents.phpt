--TEST--
svgToPng: heavy ordinary documents fit the raster work budget; charts carry none
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

/* Eight stitched 1024x512 line charts of eight 2048-point series: ~60M
 * of the 2^26 work units. The same document at 4096 px is already
 * refused by the element x pixel budget. */
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

/* Chart renders carry no raster budget: the same eight series on one
 * 4096 px chart need ~86M units. */
$info = getimagesizefromstring(line_chart(4096, 4096, 0, 8)->renderPng());
echo "chart: {$info[0]}x{$info[1]}\n";
?>
--EXPECT--
stitched charts: 2048x2048
chart: 4096x4096
