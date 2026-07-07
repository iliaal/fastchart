--TEST--
Canvas-fill families honor setTransparentBackground() (no full-canvas rect)
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php

/* 24 chart families hand-rolled their canvas fill with an unconditional
 * full-canvas opaque rect, so setTransparentBackground() (and bg-image
 * compositing / setPlotRect) were silently ignored. All now route the
 * canvas fill through fastchart_paint_canvas_bg(). A transparent render
 * must NOT emit the full-canvas <rect x="0" y="0" width=W height=H>. */

function canvas_rect(string $svg, int $w, int $h): bool {
    return (bool)preg_match(
        '/<rect x="0" y="0" width="' . $w . '" height="' . $h .
        '" fill="#[0-9A-Fa-f]{6}"\/>/', $svg);
}

$families = [
    'funnel'       => [300, 300, fn() => (new FastChart\Funnel(300, 300))
        ->setStages([['value' => 10], ['value' => 6], ['value' => 3]])],
    'gauge'        => [420, 300, fn() => (new FastChart\GaugeChart(420, 300))
        ->setRange(0, 100)->setValue(72)],
    'heatmap'      => [560, 320, fn() => (new FastChart\Heatmap(560, 320))
        ->setGrid([[1, 2, 3], [4, 5, 6], [7, 8, 9]])],
    'radar'        => [420, 420, fn() => (new FastChart\RadarChart(420, 420))
        ->setSeries([['label' => 'A', 'data' => [80, 70, 60, 85, 75]]])
        ->setCategoryLabels(['Speed', 'Power', 'Endur', 'Skill', 'Mind'])],
    'sunburst'     => [420, 420, fn() => (new FastChart\SunburstChart(420, 420))
        ->setHierarchy(['label' => 'root', 'children' => [
            ['label' => 'A', 'value' => 20], ['label' => 'B', 'value' => 20],
            ['label' => 'C', 'value' => 20]]])],
    'treemap'      => [480, 320, fn() => (new FastChart\Treemap(480, 320))
        ->setItems([['label' => 'Alpha', 'value' => 100],
            ['label' => 'Bravo', 'value' => 70],
            ['label' => 'Charlie', 'value' => 40]])],
    'arc'          => [400, 200, fn() => (new FastChart\ArcDiagram(400, 200))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 2],
            ['from' => 1, 'to' => 2, 'value' => 3]])],
    'bullet'       => [400, 80, fn() => (new FastChart\BulletChart(400, 80))
        ->setRange(0, 100)->setValue(50)],
    'calendar'     => [400, 200, fn() => (new FastChart\CalendarHeatmap(400, 200))
        ->setData(['2024-01-01' => 1.0, '2024-06-15' => 3.0, '2024-12-31' => 2.0])],
    'chord'        => [300, 300, fn() => (new FastChart\ChordDiagram(300, 300))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 2],
            ['from' => 1, 'to' => 2, 'value' => 3]])],
    'circlepack'   => [300, 300, fn() => (new FastChart\CirclePacking(300, 300))
        ->setHierarchy(['children' => [['label' => 'a', 'value' => 5],
            ['value' => 3], ['value' => 8]]])],
    'contour'      => [220, 220, fn() => (new FastChart\ContourChart(220, 220))
        ->setGrid([[0, 1, 2], [1, 2, 3], [2, 3, 4]])],
    'dendrogram'   => [300, 300, fn() => (new FastChart\Dendrogram(300, 300))
        ->setHierarchy(['children' => [['label' => 'a', 'value' => 5],
            ['value' => 3], ['value' => 8]]])],
    'linear_meter' => [480, 160, fn() => (new FastChart\LinearMeter(480, 160))
        ->setRange(0, 100)->setValue(72)],
    'marimekko'    => [400, 300, fn() => (new FastChart\MarimekkoChart(400, 300))
        ->setColumns([
            ['label' => 'Good', 'segments' => [['label' => 'A', 'value' => 10],
                ['label' => 'B', 'value' => 20]]],
            ['label' => 'Better', 'segments' => [['label' => 'A', 'value' => 15],
                ['label' => 'B', 'value' => 25]]]])],
    'network'      => [300, 300, fn() => (new FastChart\NetworkChart(300, 300))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 2],
            ['from' => 1, 'to' => 2, 'value' => 3]])],
    'pareto'       => [400, 300, fn() => (new FastChart\ParetoChart(400, 300))
        ->setBars([['label' => 'A', 'value' => 50], ['label' => 'B', 'value' => 30],
            ['label' => 'C', 'value' => 20]])],
    'partition'    => [300, 300, fn() => (new FastChart\Partition(300, 300))
        ->setHierarchy(['children' => [['label' => 'a', 'value' => 5],
            ['value' => 3], ['value' => 8]]])],
    'pictogram'    => [500, 250, fn() => (new FastChart\Pictogram(500, 250))
        ->setTotal(10)->setValue(7.5)->setIconCount(10)],
    'polar'        => [200, 200, fn() => (new FastChart\PolarChart(200, 200))
        ->setSeries([['data' => [[0, 5], [45, 8], [90, 6], [135, 7]]]])],
    'pyramid'      => [600, 400, fn() => (new FastChart\PopulationPyramid(600, 400))
        ->setCategories(['0-9', '10-19', '20-29'])
        ->setLeftSeries(['label' => 'Male', 'data' => [12, 18, 22]])
        ->setRightSeries(['label' => 'Female', 'data' => [11, 17, 21]])],
    'sankey'       => [600, 300, fn() => (new FastChart\SankeyChart(600, 300))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 2, 'value' => 5],
            ['from' => 1, 'to' => 2, 'value' => 3]])],
    'venn'         => [500, 400, fn() => (new FastChart\VennDiagram(500, 400))
        ->setSets([['label' => 'A', 'size' => 100], ['label' => 'B', 'size' => 100]])
        ->setIntersections([['sets' => [0, 1], 'size' => 30]])],
    'violin'       => [300, 300, fn() => (new FastChart\ViolinPlot(300, 300))
        ->setGroups([['label' => 'X', 'values' => [1, 2, 3, 4, 3, 2]],
            ['label' => 'Y', 'values' => [2, 4, 6, 4, 2]]])],
];

foreach ($families as $key => [$w, $h, $make]) {
    $opaque      = $make()->renderSvg();
    $transparent = $make()->setTransparentBackground(true)->renderSvg();
    $ctrl = canvas_rect($opaque, $w, $h);       /* control: must be present */
    $trns = canvas_rect($transparent, $w, $h);  /* fix: must be absent */
    echo $key, ': ', ($ctrl && !$trns) ? 'ok' : "BAD(ctrl=$ctrl trns=$trns)", "\n";
}

/* PNG corner-alpha spot-check (mirrors tests/291): a transparent render
 * leaves the canvas corner fully transparent (gd alpha 127); the opaque
 * default paints it solid (alpha 0). */
function corner_alpha(string $png): int {
    $im = imagecreatefromstring($png);
    return (imagecolorat($im, 2, 2) >> 24) & 0x7F;
}

$g_t = (new FastChart\GaugeChart(200, 160))->setRange(0, 100)->setValue(50)
    ->setTransparentBackground(true)->renderPng();
echo 'gauge png transparent: ', corner_alpha($g_t) === 127 ? 'ok' : 'BAD', "\n";

$t_t = (new FastChart\Treemap(200, 160))
    ->setItems([['label' => 'A', 'value' => 5], ['label' => 'B', 'value' => 3]])
    ->setTransparentBackground(true)->renderPng();
echo 'treemap png transparent: ', corner_alpha($t_t) === 127 ? 'ok' : 'BAD', "\n";

$g_o = (new FastChart\GaugeChart(200, 160))->setRange(0, 100)->setValue(50)
    ->renderPng();
echo 'gauge png opaque: ', corner_alpha($g_o) === 0 ? 'ok' : 'BAD', "\n";

echo "done\n";
?>
--EXPECT--
funnel: ok
gauge: ok
heatmap: ok
radar: ok
sunburst: ok
treemap: ok
arc: ok
bullet: ok
calendar: ok
chord: ok
circlepack: ok
contour: ok
dendrogram: ok
linear_meter: ok
marimekko: ok
network: ok
pareto: ok
partition: ok
pictogram: ok
polar: ok
pyramid: ok
sankey: ok
venn: ok
violin: ok
gauge png transparent: ok
treemap png transparent: ok
gauge png opaque: ok
done
