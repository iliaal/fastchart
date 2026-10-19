--TEST--
svgToPng: gradient-heavy ordinary documents fit the paint budget
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

/* Sixteen full-canvas translucent radial-gradient layers under a
 * two-series line chart at 4096 px: ~5.4G of the 2^33 paint units. */
$layers = [];
for ($i = 0; $i < 16; $i++) {
    $layers[] = '<rect width="4096" height="4096" fill="url(#r)"/>';
}
$chart = (new FastChart\LineChart(4096, 4096))
    ->setMarkerStyle(FastChart\Chart::MARKER_NONE)
    ->setSeries([series(0), series(1)]);
$svg = '<svg xmlns="http://www.w3.org/2000/svg" width="4096" height="4096">'
    . '<defs><radialGradient id="r" cx="0.3" cy="0.4" r="0.6">'
    . '<stop offset="0" stop-color="#fff" stop-opacity="0.3"/>'
    . '<stop offset="1" stop-color="#036" stop-opacity="0.3"/>'
    . '</radialGradient></defs>' . implode('', $layers)
    . $chart->drawSvgFragment('c') . '</svg>';
$info = getimagesizefromstring(FastChart\Chart::svgToPng($svg));
echo "gradient layers: {$info[0]}x{$info[1]}\n";
?>
--EXPECT--
gradient layers: 4096x4096
