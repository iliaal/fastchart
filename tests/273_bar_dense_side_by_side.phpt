--TEST--
BarChart: dense side-by-side slots never emit negative SVG dimensions
--EXTENSIONS--
fastchart
--FILE--
<?php

use FastChart\BarChart;

function dense_series(): array {
    $series = [];
    for ($s = 0; $s < 8; $s++) {
        $series[] = ['label' => "S$s", 'data' => array_fill(0, 32, 1)];
    }
    return $series;
}

function dense_floating_series(): array {
    $series = [];
    for ($s = 0; $s < 8; $s++) {
        $data = [];
        for ($i = 0; $i < 32; $i++) {
            $data[] = [0, 1];
        }
        $series[] = ['label' => "S$s", 'data' => $data];
    }
    return $series;
}

function has_negative_dimension(string $svg): bool {
    return preg_match('/(?:width|height)="-/', $svg) === 1;
}

foreach ([
    'vertical_plain' => (new BarChart(120, 120))
        ->setSeries(dense_series()),
    'horizontal_plain' => (new BarChart(120, 120))
        ->setOrientation(BarChart::BAR_HORIZONTAL)
        ->setSeries(dense_series()),
    'vertical_floating' => (new BarChart(120, 120))
        ->setFloating(true)
        ->setSeries(dense_floating_series()),
    'horizontal_floating' => (new BarChart(120, 120))
        ->setFloating(true)
        ->setOrientation(BarChart::BAR_HORIZONTAL)
        ->setSeries(dense_floating_series()),
] as $name => $chart) {
    echo "$name negative dims: ",
        (has_negative_dimension($chart->renderSvg()) ? 'FOUND' : 'none'), "\n";
}

?>
--EXPECT--
vertical_plain negative dims: none
horizontal_plain negative dims: none
vertical_floating negative dims: none
horizontal_floating negative dims: none
