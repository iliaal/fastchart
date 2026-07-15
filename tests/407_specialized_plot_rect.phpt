--TEST--
Every bespoke chart renderer positions its data inside setPlotRect bounds
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

$families = [
    'arc' => fn() => (new FastChart\ArcDiagram(240, 180))
        ->setNodes([['color' => 0xff0000], ['color' => 0xff0000], ['color' => 0xff0000]])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 2], ['from' => 1, 'to' => 2, 'value' => 3]]),
    'bullet' => fn() => (new FastChart\BulletChart(240, 180))->setRange(0, 100)->setValue(60),
    'calendar' => fn() => (new FastChart\CalendarHeatmap(240, 180))
        ->setData(['2026-01-01' => 1, '2026-01-08' => 2])->setColorRamp(0xff0000, 0xff0000),
    'chord' => fn() => (new FastChart\ChordDiagram(240, 180))
        ->setNodes([['color' => 0xff0000], ['color' => 0xff0000], ['color' => 0xff0000]])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 2], ['from' => 1, 'to' => 2, 'value' => 3]]),
    'circlepack' => fn() => (new FastChart\CirclePacking(240, 180))
        ->setHierarchy(['children' => [['value' => 5], ['value' => 3], ['value' => 8]]]),
    'contour' => fn() => (new FastChart\ContourChart(240, 180))
        ->setGrid([[0, 1, 2], [1, 2, 3], [2, 3, 4]])->setFilled(true)
        ->setColorRamp(0xff0000, 0xff0000),
    'dendrogram' => fn() => (new FastChart\Dendrogram(240, 180))
        ->setHierarchy(['children' => [['value' => 5], ['value' => 3], ['value' => 8]]]),
    'funnel' => fn() => (new FastChart\Funnel(240, 180))
        ->setStages([['value' => 10, 'color' => 0xff0000], ['value' => 6, 'color' => 0xff0000]]),
    'gauge' => fn() => (new FastChart\GaugeChart(240, 180))->setRange(0, 100)->setValue(72),
    'linear' => fn() => (new FastChart\LinearMeter(240, 180))->setRange(0, 100)->setValue(72)
        ->setZones([['from' => 0, 'to' => 100, 'color' => 0xff0000]]),
    'marimekko' => fn() => (new FastChart\MarimekkoChart(240, 180))->setColumns([
        ['segments' => [['value' => 10, 'color' => 0xff0000], ['value' => 20, 'color' => 0xff0000]]],
        ['segments' => [['value' => 15, 'color' => 0xff0000], ['value' => 25, 'color' => 0xff0000]]],
    ]),
    'network' => fn() => (new FastChart\NetworkChart(240, 180))
        ->setNodes([['color' => 0xff0000], ['color' => 0xff0000], ['color' => 0xff0000]])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 2], ['from' => 1, 'to' => 2, 'value' => 3]]),
    'pareto' => fn() => (new FastChart\ParetoChart(240, 180))
        ->setBars([['value' => 50, 'color' => 0xff0000], ['value' => 30, 'color' => 0xff0000]]),
    'partition' => fn() => (new FastChart\Partition(240, 180))
        ->setHierarchy(['children' => [['value' => 5, 'color' => 0xff0000],
            ['value' => 3, 'color' => 0xff0000]]]),
    'pictogram' => fn() => (new FastChart\Pictogram(240, 180))->setTotal(10)->setValue(7.5)
        ->setIconCount(10)->setFillColor(0xff0000),
    'polar' => fn() => (new FastChart\PolarChart(240, 180))
        ->setSeries([['color' => 0xff0000, 'data' => [[0, 5], [90, 8], [180, 6], [270, 7]]]]),
    'pyramid' => fn() => (new FastChart\PopulationPyramid(240, 180))
        ->setCategories(['a', 'b'])->setLeftSeries(['data' => [12, 18]])
        ->setRightSeries(['data' => [11, 17]]),
    'radar' => fn() => (new FastChart\RadarChart(240, 180))
        ->setSeries([['color' => 0xff0000, 'data' => [80, 70, 60, 85, 75]]]),
    'sankey' => fn() => (new FastChart\SankeyChart(240, 180))
        ->setNodes([['color' => 0xff0000], ['color' => 0xff0000], ['color' => 0xff0000]])
        ->setLinks([['from' => 0, 'to' => 2, 'value' => 5], ['from' => 1, 'to' => 2, 'value' => 3]]),
    'sunburst' => fn() => (new FastChart\SunburstChart(240, 180))
        ->setHierarchy(['children' => [['value' => 20, 'color' => 0xff0000],
            ['value' => 20, 'color' => 0xff0000]]]),
    'surface' => fn() => (new FastChart\SurfaceChart(240, 180))
        ->setGrid([[1, 2], [3, 4]])->setColorRamp(0xff0000, 0xff0000),
    'venn' => fn() => (new FastChart\VennDiagram(240, 180))
        ->setSets([['size' => 100, 'color' => 0xff0000], ['size' => 100, 'color' => 0xff0000]])
        ->setIntersections([['sets' => [0, 1], 'size' => 30]]),
    'violin' => fn() => (new FastChart\ViolinPlot(240, 180))
        ->setGroups([['values' => [1, 2, 3, 4, 3, 2]], ['values' => [2, 4, 6, 4, 2]]]),
    'wordcloud' => fn() => (new FastChart\WordCloud(240, 180))
        ->setWords([['text' => 'AAAA', 'weight' => 10, 'color' => 0xff0000]]),
];

foreach ($families as $name => $make) {
    $chart = $make()->setSeriesColors(array_fill(0, 8, 0xff0000))
        ->setPlotRect(20, 20, 120, 120)->setTransparentBackground(true);
    $im = imagecreatefromstring($chart->renderPng());
    $minX = $minY = PHP_INT_MAX;
    $maxX = $maxY = -1;
    $count = 0;
    for ($y = 0; $y < imagesy($im); $y++) {
        for ($x = 0; $x < imagesx($im); $x++) {
            $pixel = imagecolorat($im, $x, $y);
            $r = ($pixel >> 16) & 255;
            $g = ($pixel >> 8) & 255;
            $b = $pixel & 255;
            if ($r > 200 && $g < 50 && $b < 50) {
                $count++;
                $minX = min($minX, $x); $maxX = max($maxX, $x);
                $minY = min($minY, $y); $maxY = max($maxY, $y);
            }
        }
    }
    $bounded = $count > 20 && $minX >= 15 && $minY >= 15
        && $maxX <= 125 && $maxY <= 125;
    echo "$name: ", $bounded ? "yes\n" : "no ($count $minX,$minY-$maxX,$maxY)\n";
}
?>
--EXPECT--
arc: yes
bullet: yes
calendar: yes
chord: yes
circlepack: yes
contour: yes
dendrogram: yes
funnel: yes
gauge: yes
linear: yes
marimekko: yes
network: yes
pareto: yes
partition: yes
pictogram: yes
polar: yes
pyramid: yes
radar: yes
sankey: yes
sunburst: yes
surface: yes
venn: yes
violin: yes
wordcloud: yes
