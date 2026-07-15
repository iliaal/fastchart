--TEST--
BarChart omits image-map hotspots for categories containing only gaps
--EXTENSIONS--
fastchart
--FILE--
<?php

foreach ([FastChart\BarChart::BAR_VERTICAL, FastChart\BarChart::BAR_HORIZONTAL] as $orientation) {
    $chart = (new FastChart\BarChart(300, 200))
        ->setOrientation($orientation)
        ->setSeries([NAN, 2])
        ->setImageMap([['href' => '/gap'], ['href' => '/ok']]);
    $chart->renderSvg();
    $areas = $chart->getImageMapAreas();
    echo $orientation === FastChart\BarChart::BAR_VERTICAL ? 'vertical: ' : 'horizontal: ';
    echo count($areas) === 1 && $areas[0]['href'] === '/ok' ? "yes\n" : "no\n";
}

?>
--EXPECT--
vertical: yes
horizontal: yes
