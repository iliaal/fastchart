<?php

/* Image-map migration: scatter renders, getImageMap returns the
 * <map> markup, repeated draws/clones don't UAF. */
$chart = (new FastChart\ScatterChart(400, 200))
    ->setPoints([
        [1.0, 10.0, 'href' => '/a', 'tooltip' => 'pt1'],
        [2.0, 20.0, 'href' => 'https://example.com/b'],
        [3.0, 15.0, 'href' => 'mailto:c@example.com'],
        [4.0, 25.0],  // no href, skipped from <area>
    ]);
$im = imagecreatetruecolor(400, 200);
$chart->draw($im);
$map = $chart->getImageMap('test');
echo "image_map_areas: ", (
    str_contains($map, '<map name="test">') &&
    substr_count($map, '<area ') === 3 &&
    str_contains($map, 'href="/a"') &&
    str_contains($map, 'href="https://example.com/b"') &&
    str_contains($map, 'href="mailto:c@example.com"') &&
    str_contains($map, 'title="pt1"') &&
    str_ends_with($map, '</map>')
    ? "ok" : "bad"
), "\n";

/* Image-map clone path: clone before draw → cloned chart has no
 * areas yet; cloned chart's draw repopulates fresh. */
$c2 = clone $chart;
$im2 = imagecreatetruecolor(400, 200);
$c2->draw($im2);
$map2 = $c2->getImageMap();
echo "image_map_clone: ", (substr_count($map2, '<area ') === 3 ? "ok" : "bad"), "\n";

/* Repeated draws: the area list is regenerated per draw, no leak. */
for ($i = 0; $i < 5; $i++) {
    $chart->draw($im);
}
echo "image_map_repeat: ", (substr_count($chart->getImageMap(), '<area ') === 3 ? "ok" : "bad"), "\n";

/* getImageMap before any draw returns empty. */
$fresh = new FastChart\ScatterChart(400, 200);
echo "image_map_undrawn: ", ($fresh->getImageMap() === '' ? "ok" : "bad"), "\n";

/* dangerous href schemes are rejected silently. */
$bad = (new FastChart\ScatterChart(400, 200))
    ->setPoints([
        [1.0, 10.0, 'href' => 'javascript:alert(1)'],
        [2.0, 20.0, 'href' => 'data:text/html,<x>'],
        [3.0, 15.0, 'href' => '/safe'],
    ]);
$bad->draw(imagecreatetruecolor(400, 200));
$bm = $bad->getImageMap();
echo "image_map_scheme_filter: ", (
    !str_contains($bm, 'javascript:') &&
    !str_contains($bm, 'data:') &&
    str_contains($bm, '/safe')
    ? "ok" : "bad"
), "\n";

/* Scatter X label format: setXAxisLabelFormat now applies on
 * scatter ticks (was hardcoded "%.*f" before). Smoke-test that the
 * setter accepts and the chart renders. */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\ScatterChart(400, 200))
    ->setPoints([[1.0, 10.0], [5.5, 25.0], [10.0, 18.0]])
    ->setXAxisLabelFormat('%.2f')
    ->draw($im);
echo "scatter_x_format: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* addOverlaySeries 'label' option is now silently dropped (was dead
 * code with a docstring claim). The setter accepts the option but
 * ignores it; renderer still works. */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\LineChart(400, 200))
    ->setSeries([['data' => [10, 20, 30, 25]]])
    ->addOverlaySeries('line', [12, 22, 28, 24],
                       ['label' => 'ignored', 'color' => 0xFF0000])
    ->draw($im);
echo "overlay_no_label: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* Trend-line degree validation matches the corrected stub doc:
 * 1..3 accepted, 4+ rejected. */
$pts = [];
for ($i = 0; $i < 20; $i++) $pts[] = [$i, $i * 2.0];
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\ScatterChart(400, 200))
    ->setPoints($pts)
    ->setTrendLine(true, null, 3)
    ->draw($im);
echo "trend_degree_3: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

try {
    (new FastChart\ScatterChart(400, 200))
        ->setPoints($pts)
        ->setTrendLine(true, null, 4);
    echo "trend_degree_4: no throw\n";
} catch (\ValueError $e) {
    echo "trend_degree_4: ValueError ok\n";
}

/* Per-render font cache: render twice with show_values; should pass
 * (smoke test for cache invalidation between renders). */
$im1 = imagecreatetruecolor(400, 200);
$im2 = imagecreatetruecolor(400, 200);
$c = (new FastChart\BarChart(400, 200))
    ->setSeries([['data' => [10, 20, 30, 25]]])
    ->setShowValues(true);
$c->draw($im1);
$c->draw($im2);
echo "font_cache_repeat: ok\n";
?>
