--TEST--
Hierarchy labels and text annotations enforce aggregate render-text budgets
--EXTENSIONS--
fastchart
--FILE--
<?php

$children = [];
for ($i = 0; $i < 9; $i++) {
    $children[] = ['label' => str_repeat('W', 8192), 'value' => 1];
}

foreach ([
    FastChart\CirclePacking::class,
    FastChart\Dendrogram::class,
    FastChart\Partition::class,
] as $class) {
    $chart = (new $class(300, 200))->setHierarchy([
        'label' => 'root',
        'children' => [['label' => 'ok', 'value' => 1]],
    ]);
    $before = $chart->renderSvg();
    try {
        $chart->setHierarchy(['label' => 'root', 'children' => $children]);
        echo "$class hierarchy: accepted\n";
    } catch (ValueError $e) {
        echo "$class hierarchy: ",
            $chart->renderSvg() === $before ? "preserved\n" : "changed\n";
    }
}

$annotations = (new FastChart\LineChart(300, 200))
    ->setSeries([1, 2])
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
for ($i = 0; $i < 8; $i++) {
    $annotations->addTextAnnotation(str_repeat('A', 8192), 10, 10 + $i);
}
try {
    $annotations->addTextAnnotation('x', 10, 20);
    echo "annotations: accepted\n";
} catch (ValueError $e) {
    echo 'annotations retained: ',
        substr_count($annotations->renderSvg(), str_repeat('A', 8192)) === 8
            ? "yes\n" : "no\n";
}

$counted = (new FastChart\LineChart(300, 200))->setSeries([1, 2]);
for ($i = 0; $i < 127; $i++) {
    $counted->addTextAnnotation('', 0, 0);
}
$counted = clone $counted;
$counted->addTextAnnotation('', 0, 0);
try {
    $counted->addTextAnnotation('', 0, 0);
    echo "annotation clone count: no\n";
} catch (ValueError $e) {
    echo "annotation clone count: yes\n";
}

?>
--EXPECT--
FastChart\CirclePacking hierarchy: preserved
FastChart\Dendrogram hierarchy: preserved
FastChart\Partition hierarchy: preserved
annotations retained: yes
annotation clone count: yes
