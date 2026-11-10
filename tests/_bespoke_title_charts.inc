<?php
// Small valid fixtures shared by the native SVG and raster/path title tests.
function bespokeTitleCharts(): array {
    $charts = [];
    $nodes = [['label' => 'Alpha'], ['label' => 'Beta']];
    $links = [['from' => 0, 'to' => 1, 'value' => 3]];
    foreach (['ArcDiagram', 'ChordDiagram', 'NetworkChart', 'SankeyChart'] as $name) {
        $class = 'FastChart\\' . $name;
        $charts[$name] = (new $class(480, 320))->setNodes($nodes)->setLinks($links);
    }
    $tree = ['label' => 'Root', 'children' => [
        ['label' => 'Alpha', 'value' => 3], ['label' => 'Beta', 'value' => 2],
    ]];
    foreach (['CirclePacking', 'Dendrogram', 'Partition', 'SunburstChart'] as $name) {
        $class = 'FastChart\\' . $name;
        $charts[$name] = (new $class(480, 320))->setHierarchy($tree);
    }
    foreach (['BulletChart', 'LinearMeter'] as $name) {
        $class = 'FastChart\\' . $name;
        $charts[$name] = (new $class(480, 320))->setRange(0, 10)->setValue(6);
    }
    $items = [['label' => 'Alpha', 'value' => 3], ['label' => 'Beta', 'value' => 2]];
    $charts['CalendarHeatmap'] = (new FastChart\CalendarHeatmap(480, 320))
        ->setData(['2026-01-01' => 3, '2026-01-08' => 2]);
    $charts['Funnel'] = (new FastChart\Funnel(480, 320))->setStages($items);
    $charts['Treemap'] = (new FastChart\Treemap(480, 320))->setItems($items);
    $charts['ParetoChart'] = (new FastChart\ParetoChart(480, 320))->setBars($items);
    $charts['MarimekkoChart'] = (new FastChart\MarimekkoChart(480, 320))->setColumns([
        ['label' => 'First', 'segments' => $items],
        ['label' => 'Second', 'segments' => $items],
    ]);
    $charts['Pictogram'] = (new FastChart\Pictogram(480, 320))->setTotal(10)->setValue(6);
    $charts['PopulationPyramid'] = (new FastChart\PopulationPyramid(480, 320))
        ->setCategories(['Alpha', 'Beta'])
        ->setLeftSeries(['label' => 'Left', 'data' => [3, 2]])
        ->setRightSeries(['label' => 'Right', 'data' => [2, 3]]);
    $charts['SerpentineTimeline'] = (new FastChart\SerpentineTimeline(480, 320))
        ->setEvents([['label' => 'Alpha', 'date' => 'Jan'], ['label' => 'Beta', 'date' => 'Feb']]);
    $charts['VectorChart'] = (new FastChart\VectorChart(480, 320))->setVectors([
        ['x' => 0, 'y' => 0, 'dx' => 1, 'dy' => 1],
        ['x' => 3, 'y' => 3, 'dx' => 1, 'dy' => 1],
    ]);
    $charts['VennDiagram'] = (new FastChart\VennDiagram(480, 320))
        ->setSets([['label' => 'Alpha', 'size' => 3], ['label' => 'Beta', 'size' => 2]])
        ->setIntersections([['sets' => [0, 1], 'size' => 1]]);
    $charts['WordCloud'] = (new FastChart\WordCloud(480, 320))
        ->setWords([['text' => 'Alpha', 'weight' => 3], ['text' => 'Beta', 'weight' => 2]]);
    $charts['ViolinPlot'] = (new FastChart\ViolinPlot(480, 320))->setGroups([
        ['label' => 'Alpha', 'values' => [1, 2, 3, 4, 5]],
        ['label' => 'Beta', 'values' => [2, 3, 4, 5, 6]],
    ]);
    ksort($charts);
    return $charts;
}
