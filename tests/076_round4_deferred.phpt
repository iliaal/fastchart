--TEST--
round 4 deferred: protocol-relative href, setShadowAlpha range, locale-immune format dispatch
--EXTENSIONS--
fastchart
--FILE--
<?php

/* L-1: protocol-relative href is now allowed in image maps. */
$chart = (new FastChart\ScatterChart(400, 300))->setPoints([
    [1, 1, 'href' => '//cdn.example.com/a'],
    [2, 2, 'href' => 'javascript:alert(1)'],
    [3, 3, 'href' => '/relative/path'],
    [4, 4, 'href' => 'https://ok.example.com/'],
]);
$chart->renderPng();
$map = $chart->getImageMap('m');
echo str_contains($map, '//cdn.example.com/a')   ? "protocol-relative: kept\n"   : "protocol-relative: dropped\n";
echo str_contains($map, 'javascript:')           ? "javascript: kept\n"          : "javascript: dropped\n";
echo str_contains($map, '/relative/path')        ? "absolute path: kept\n"       : "absolute path: dropped\n";
echo str_contains($map, 'https://ok.example.com')? "https: kept\n"               : "https: dropped\n";

/* L-8: setShadowAlpha accepts 0..127, rejects out-of-range. */
$ok = (new FastChart\LineChart)->setShadowAlpha(0)->setShadowAlpha(127)->setShadowAlpha(64);
echo "setShadowAlpha(0/64/127): ok\n";

try {
    (new FastChart\LineChart)->setShadowAlpha(-1);
    echo "setShadowAlpha(-1): no throw\n";
} catch (\ValueError $e) {
    echo "setShadowAlpha(-1): ValueError ok\n";
}
try {
    (new FastChart\LineChart)->setShadowAlpha(128);
    echo "setShadowAlpha(128): no throw\n";
} catch (\ValueError $e) {
    echo "setShadowAlpha(128): ValueError ok\n";
}

/* Smoke: shadow with custom alpha actually renders. */
(new FastChart\BarChart(200, 100))
    ->setSeries([['data' => [1, 2, 3]]])
    ->setDropShadow(3, 3)
    ->setShadowAlpha(100)
    ->renderPng();
echo "shadow render: ok\n";

/* L-2: smoke renderToFile with mixed-case extensions still dispatches
 * via ASCII-only fold. We can't easily flip the C locale here, but we
 * can confirm the common-case JPG/PNG/JPEG paths still work. */
$tmp = sys_get_temp_dir() . '/fc_l2_' . getmypid();
foreach (['PNG', 'JPG', 'Jpeg', 'WebP'] as $ext) {
    $p = "$tmp.$ext";
    (new FastChart\LineChart(100, 50))
        ->setSeries([['data' => [1, 2]]])->renderToFile($p);
    echo "$ext: " . (file_exists($p) && filesize($p) > 0 ? "ok" : "FAIL") . "\n";
    @unlink($p);
}
?>
--EXPECT--
protocol-relative: kept
javascript: dropped
absolute path: kept
https: kept
setShadowAlpha(0/64/127): ok
setShadowAlpha(-1): ValueError ok
setShadowAlpha(128): ValueError ok
shadow render: ok
PNG: ok
JPG: ok
Jpeg: ok
WebP: ok
