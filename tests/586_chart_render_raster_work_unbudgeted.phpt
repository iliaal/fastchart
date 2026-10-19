--TEST--
Chart renders carry no raster work budget
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

/* Eight 2048-point series on one 2048x4096 chart need ~1.32G work
 * units, over the 1.2G svgTo*() budget; chart renders pass no budget. */
$info = getimagesizefromstring(line_chart(2048, 4096, 0, 8)->renderPng());
echo "chart: {$info[0]}x{$info[1]}\n";
?>
--EXPECT--
chart: 2048x4096
