--TEST--
PieChart::setSliceLabelPosition LABEL_LEFT and LABEL_RIGHT route labels to one side
--EXTENSIONS--
fastchart
--FILE--
<?php

$slices = ['A' => 30, 'B' => 25, 'C' => 20, 'D' => 15, 'E' => 10];

foreach ([
    'LEFT'  => FastChart\Chart::LABEL_LEFT,
    'RIGHT' => FastChart\Chart::LABEL_RIGHT,
] as $name => $pos) {
    $b = (new FastChart\PieChart(400, 400))
        ->setSlices($slices)
        ->setSliceLabelPosition($pos)
        ->renderPng();
    echo "$name: ", strlen($b) > 1024 ? "ok" : "bad", "\n";
}

// Out-of-range still rejected.
try {
    (new FastChart\PieChart)->setSliceLabelPosition(99);
    echo "bad: no throw\n";
} catch (\ValueError $e) {
    echo "bad: ValueError ok\n";
}
?>
--EXPECT--
LEFT: ok
RIGHT: ok
bad: ValueError ok
