--TEST--
PolarChart: the final rose wedge wraps to the first angle
--EXTENSIONS--
fastchart
--FILE--
<?php
function roseSvg(array $points): string {
    return (new FastChart\PolarChart(400, 400))
        ->setStyle(FastChart\PolarChart::STYLE_ROSE)
        ->setMaxRadius(1)
        ->setSeries([['data' => $points, 'color' => 0x123456]])
        ->renderSvg();
}

function rosePaths(array $points): array {
    $svg = roseSvg($points);
    preg_match_all('/<path d="([^"]+)" fill="#123456"\/>/', $svg, $matches);
    return $matches[1];
}

/* Rotating the input puts the closing wedge first, where its endpoint
 * is explicit. Its geometry must not depend on its position in the list.
 * The old uniform-spacing fallback left a gap or overlapped the first
 * wedge when the final angular interval differed from 360 / count. */
foreach ([
    'gap' => [[0, 1], [90, 1], [180, 1]],
    'overlap' => [[0, 1], [90, 1], [300, 1]],
    'offset' => [[30, 1], [120, 1], [210, 1]],
    'negative angles' => [[-90, 1], [0, 1], [90, 1]],
    'wraparound' => [[300, 1], [30, 1], [120, 1]],
    'uniform' => [[0, 1], [120, 1], [240, 1]],
] as $name => $points) {
    $paths = rosePaths($points);
    $last = array_pop($points);
    array_unshift($points, $last);
    $rotated = rosePaths($points);
    echo "$name: ", count($paths) === 3 && end($paths) === $rotated[0] ? "closed\n" : "wrong\n";
}

/* A singleton retains its full-circle wedge. */
$single = roseSvg([[30, 1]]);
echo 'singleton: ', str_contains($single, '<circle cx="200" cy="204" r="160" fill="#123456"/>') ? "full circle\n" : "wrong\n";

function roseDiscs(array $points): int {
    return preg_match_all('/<circle [^>]*fill="#123456"\/>/', roseSvg($points));
}

/* A bearing congruent to its successor modulo 360 spans no angle, so its
 * wedge is dropped instead of painting a full disc over its neighbours.
 * The surviving wedges must match the series without the duplicate. */
foreach ([
    'closing duplicate' => [
        [[0, 1], [90, .5], [180, .5], [270, .5], [360, 1]],
        [[0, 1], [90, .5], [180, .5], [270, .5]],
    ],
    'mid-series duplicate' => [
        [[0, 1], [90, .5], [450, .8], [180, .5], [270, .5]],
        [[0, 1], [90, .8], [180, .5], [270, .5]],
    ],
    'sub-degree sweep' => [
        [[0.4, 1], [0.8, .5], [180, .5]],
        [[0.8, .5], [180, .5]],
    ],
] as $name => [$points, $expected]) {
    $ok = roseDiscs($points) === 0 && rosePaths($points) === rosePaths($expected);
    echo "$name: ", $ok ? "no disc\n" : "wrong\n";
}

/* A sweep just short of 360 degrees truncates to a full turn and keeps
 * its disc; the sub-degree closing wedge is dropped. */
$near = roseSvg([[0, 1], [359.6, .5]]);
echo 'near-full sweep: ', roseDiscs([[0, 1], [359.6, .5]]) === 1
    && str_contains($near, '<circle cx="200" cy="204" r="160" fill="#123456"/>')
    && rosePaths([[0, 1], [359.6, .5]]) === [] ? "full circle\n" : "wrong\n";
?>
--EXPECT--
gap: closed
overlap: closed
offset: closed
negative angles: closed
wraparound: closed
uniform: closed
singleton: full circle
closing duplicate: no disc
mid-series duplicate: no disc
sub-degree sweep: no disc
near-full sweep: full circle
