--TEST--
A caught over-cap ValueError leaves the prior chart state renderable
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Each setter validates input caps BEFORE destroying committed state,
 * so catching the ValueError must leave the previously-set data intact
 * and byte-identical on re-render. Covers top-level caps (parse-into-
 * self and parse-into-temp shapes) and per-entry nested caps. */

function survives(string $label, object $chart, callable $overcap): void {
    $before = $chart->renderSvg();
    try {
        $overcap($chart);
        echo "$label: NO THROW\n";
        return;
    } catch (\ValueError $e) {
        // expected
    }
    $after = $chart->renderSvg();
    echo "$label: ", ($after === $before ? "survives" : "CORRUPTED"), "\n";
}

survives('pie_slices',
    (new FastChart\PieChart(300, 300))->setSlices(['A' => 40, 'B' => 60]),
    fn($c) => $c->setSlices(array_fill(0, 33, ['value' => 1])));

survives('pie_rings_nested',
    (new FastChart\PieChart(320, 320))->setRings([[['value' => 1], ['value' => 2]]]),
    fn($c) => $c->setRings([array_fill(0, 33, ['value' => 1])]));

survives('wordcloud',
    (new FastChart\WordCloud(300, 200))->setWords([['text' => 'hi', 'weight' => 1]]),
    fn($c) => $c->setWords(array_fill(0, 257, ['text' => 'x', 'weight' => 1])));

survives('gauge_zones',
    (new FastChart\GaugeChart(300, 220))->setRange(0, 100)->setValue(50)
        ->setZones([['from' => 0, 'to' => 100, 'color' => 0x33AA66]]),
    fn($c) => $c->setZones(array_fill(0, 17, ['from' => 0, 'to' => 1])));

survives('vector',
    (new FastChart\VectorChart(300, 300))->setVectors([['x' => 0, 'y' => 0, 'dx' => 1, 'dy' => 1]]),
    fn($c) => $c->setVectors(array_fill(0, 4097, ['x' => 0, 'y' => 0, 'dx' => 1, 'dy' => 1])));

survives('sankey_nodes',
    (new FastChart\SankeyChart(400, 240))
        ->setNodes([['label' => 'a'], ['label' => 'b']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 4]]),
    fn($c) => $c->setNodes(array_fill(0, 257, ['label' => 'n'])));

survives('gantt_nested_deps',
    (new FastChart\GanttChart(420, 200))->setTasks([
        ['name' => 'a', 'start' => 0, 'end' => 5, 'depends' => [0]]]),
    fn($c) => $c->setTasks([
        ['name' => 'b', 'start' => 0, 'end' => 5, 'depends' => array_fill(0, 65, 0)]]));

survives('boxplot_nested_outliers',
    (new FastChart\BoxPlot(400, 220))->setBoxes([
        ['label' => 'a', 'min' => 0, 'q1' => 1, 'median' => 2, 'q3' => 3, 'max' => 4]]),
    fn($c) => $c->setBoxes([
        ['label' => 'b', 'min' => 0, 'q1' => 1, 'median' => 2, 'q3' => 3, 'max' => 4,
         'outliers' => array_fill(0, 129, 9.0)]]));

survives('violin_nested_values',
    (new FastChart\ViolinPlot(420, 260))->setGroups([['label' => 'a', 'values' => [1, 2, 3, 4]]]),
    fn($c) => $c->setGroups([['label' => 'b', 'values' => array_fill(0, 8193, 1.0)]]));

survives('marimekko_nested_segs',
    (new FastChart\MarimekkoChart(420, 260))->setColumns([
        ['label' => 'a', 'segments' => [['label' => 's', 'value' => 1]]]]),
    fn($c) => $c->setColumns([
        ['label' => 'b', 'segments' => array_fill(0, 65, ['label' => 's', 'value' => 1])]]));

/* Bar clears stale image-map hot-spots when a later render aborts on a
 * validation error, rather than leaving the prior render's areas. */
$bar = (new FastChart\BarChart(300, 200))->setSeries([1, 2, 3])->setImageMap([['href' => '/a']]);
$bar->renderSvg();
$bar->setYAxisScale(FastChart\Chart::SCALE_LOG);
$bar->setSeries([0, -1, 2]);
try { $bar->renderSvg(); } catch (\Throwable $e) {}
echo "bar stale areas cleared: ", (count($bar->getImageMapAreas()) === 0 ? "yes" : "NO"), "\n";

/* Alpha 127 (libgd fully transparent) must not leak a 0.004 opacity. */
$band = (new FastChart\LineChart(300, 200))->setSeries([1, 2, 3])
    ->addHorizontalBand(1, 2, 0xFF0000, 127)->renderSvg();
echo "alpha127 fully transparent: ", (str_contains($band, ',0.004') ? "NO" : "yes"), "\n";

/* getImageMap hot-spots: Bar/Pie/Scatter populate; others do not
 * (matches the corrected stub doc). */
function area_count(object $c): int {
    $c->renderSvg();
    return count($c->getImageMapAreas());
}
echo "bar hotspots: ", (area_count((new FastChart\BarChart(300, 200))
    ->setSeries([1, 2])->setImageMap([['href' => '/a'], ['href' => '/b']])) > 0 ? "yes" : "no"), "\n";
echo "line hotspots: ", (area_count((new FastChart\LineChart(300, 200))
    ->setSeries([1, 2])->setImageMap([['href' => '/a'], ['href' => '/b']])) > 0 ? "yes" : "no"), "\n";

?>
--EXPECT--
pie_slices: survives
pie_rings_nested: survives
wordcloud: survives
gauge_zones: survives
vector: survives
sankey_nodes: survives
gantt_nested_deps: survives
boxplot_nested_outliers: survives
violin_nested_values: survives
marimekko_nested_segs: survives
bar stale areas cleared: yes
alpha127 fully transparent: yes
bar hotspots: yes
line hotspots: no
