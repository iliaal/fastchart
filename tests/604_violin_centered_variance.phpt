--TEST--
ViolinPlot bandwidth preserves small spreads around large finite offsets
--EXTENSIONS--
fastchart
--FILE--
<?php
function chart(array $values): FastChart\ViolinPlot {
    return (new FastChart\ViolinPlot(600, 400))->setGroups([
        ['color' => 0x123456, 'values' => $values],
    ]);
}
// Both sides of subsampling transitions, plus the per-group input cap.
foreach ([1023, 1024, 1025, 2047, 2048, 2049, 8192] as $n) {
    foreach ([1e12 - 0.01, -1e12] as $low) {
        $high = $low + 0.01;
        $nl = intdiv($n, 2);
        $nh = $n - $nl;
        $values = array_merge(array_fill(0, $nl, $low), array_fill(0, $nh, $high));
        $original = chart($values);
        $svg = $original->renderSvg();
        $stable = $svg === chart(array_reverse($values))->renderSvg()
            && $svg === (clone $original)->renderSvg() && $svg === $original->renderSvg();
        // Closed-form sample deviation for a two-point distribution avoids
        // reconstructing a rounded mean near the large common offset.
        $range = $high - $low;
        $sd = $range * sqrt(($nl * $nh) / ($n * ($n - 1)));
        $h = 1.06 * $sd * pow($n, -0.2);
        if ($h < $range * 0.01) $h = $range * 0.02;
        $stride = $n > 1024 ? intdiv($n + 1023, 1024) : 1;
        $sampledLow = intdiv($nl + $stride - 1, $stride);
        $sampled = intdiv($n + $stride - 1, $stride);
        $densities = [];
        for ($j = 0; $j < 48; $j++) {
            $x = $low + $range * ($j / 47);
            $densities[] = $sampledLow * exp(-0.5 * (($x - $low) / $h) ** 2)
                + ($sampled - $sampledLow) * exp(-0.5 * (($x - $high) / $h) ** 2);
        }
        $max = max($densities);
        $expected = [];
        foreach ([1, -1] as $side) {
            foreach ($side === 1 ? range(0, 47) : range(47, 0) as $j) {
                $x = $low + $range * ($j / 47);
                $expected[] = [(int)(312 + $side * $densities[$j] / $max * 231.2),
                    (int)(372 - ($x - $low) / $range * 348)];
            }
        }
        preg_match('/<polygon[^>]*points="([^"]+)"[^>]*fill="#123456"/', $svg, $m);
        $points = array_map(fn($p) => array_map('intval', explode(',', $p)),
            preg_split('/\s+/', trim($m[1] ?? '')));
        $accurate = count($points) === 96;
        foreach ($expected as $i => $point) {
            // One pixel allows equivalent floating-point expression rounding.
            $accurate = $accurate && isset($points[$i])
                && abs($points[$i][0] - $point[0]) <= 1
                && abs($points[$i][1] - $point[1]) <= 1;
        }
        echo "$n ", $low > 0 ? 'positive' : 'negative', ': ',
            $stable && $accurate ? 'OK' : 'FAIL', "\n";
    }
}
?>
--EXPECT--
1023 positive: OK
1023 negative: OK
1024 positive: OK
1024 negative: OK
1025 positive: OK
1025 negative: OK
2047 positive: OK
2047 negative: OK
2048 positive: OK
2048 negative: OK
2049 positive: OK
2049 negative: OK
8192 positive: OK
8192 negative: OK
