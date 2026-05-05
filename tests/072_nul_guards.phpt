--TEST--
String setters reject embedded NUL; getImageMap drops <area> on NUL href and renames empty map names
--EXTENSIONS--
fastchart
--FILE--
<?php

// setTitle / setXAxisTitle / setYAxisTitle / setSecondaryYAxisTitle /
// PieChart::setOtherThreshold($p, $label) all reject embedded NUL.
foreach ([
    ['setTitle',                ["bad\0title"]],
    ['setXAxisTitle',           ["bad\0x"]],
    ['setYAxisTitle',           ["bad\0y"]],
    ['setSecondaryYAxisTitle',  ["bad\0y2"]],
] as [$method, $args]) {
    try {
        (new FastChart\LineChart)->$method(...$args);
        echo "$method: no throw\n";
    } catch (\ValueError $e) {
        echo "$method: ValueError ok\n";
    }
}

try {
    (new FastChart\PieChart)->setOtherThreshold(5.0, "other\0evil");
    echo "PieChart::setOtherThreshold: no throw\n";
} catch (\ValueError $e) {
    echo "PieChart::setOtherThreshold: ValueError ok\n";
}

// getImageMap with NUL in href drops the <area>; with NUL in title drops
// the title attr but keeps the link.
$c = new FastChart\ScatterChart(400, 300);
$c->setPoints([
    [1.0, 2.0, 'href' => "/safe\0evil",          'tooltip' => 'safe-tip'],
    [2.0, 3.0, 'href' => "/ok",                   'tooltip' => "tip\0evil"],
    [3.0, 4.0, 'href' => "/keep",                 'tooltip' => "tip-keep"],
])->renderPng();
$map = $c->getImageMap('test');
echo "nul_href_dropped: ",  (str_contains($map, "/safe") ? "FAIL" : "ok"), "\n";
echo "nul_tip_link_kept: ", (str_contains($map, 'href="/ok"') ? "ok" : "FAIL"), "\n";
echo "nul_tip_dropped: ",   (str_contains($map, "tip\0evil") || str_contains($map, "evil") ? "FAIL" : "ok"), "\n";
echo "ok_entry_kept: ",     (str_contains($map, 'href="/keep"') && str_contains($map, 'tip-keep') ? "ok" : "FAIL"), "\n";

// Empty-after-sanitize map name falls back to "fastchart".
$c2 = (new FastChart\ScatterChart(200, 150));
$c2->setPoints([[1, 2, 'href' => '/x']])->renderPng();
$map2 = $c2->getImageMap("!@#%^&*");
echo "empty_name_fallback: ", (str_contains($map2, 'name="fastchart"') ? "ok" : "FAIL"), "\n";
?>
--EXPECT--
setTitle: ValueError ok
setXAxisTitle: ValueError ok
setYAxisTitle: ValueError ok
setSecondaryYAxisTitle: ValueError ok
PieChart::setOtherThreshold: ValueError ok
nul_href_dropped: ok
nul_tip_link_kept: ok
nul_tip_dropped: ok
ok_entry_kept: ok
empty_name_fallback: ok
