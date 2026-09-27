--TEST--
SunburstChart rejects aggregates outside the finite numeric range
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

use FastChart\SunburstChart;

$chart = (new SunburstChart(400, 400))->setHierarchy([
    'label' => 'root',
    'children' => [
        ['label' => 'a', 'value' => 1, 'color' => 0xFF0000],
        ['label' => 'b', 'value' => 1, 'color' => 0x00FF00],
    ],
]);
$before = $chart->renderSvg();

try {
    $chart->setHierarchy([
        'children' => [
            ['label' => 'a', 'value' => 1e308],
            ['label' => 'b', 'value' => 1e308],
        ],
    ]);
    echo "extreme: accepted\n";
} catch (ValueError $e) {
    echo "extreme: ", $e->getMessage(), "\n";
}

try {
    $chart->setHierarchy([
        'value' => 1.0,
        'children' => [
            ['label' => 'a', 'value' => 1e308],
            ['label' => 'b', 'value' => 1e308],
        ],
    ]);
    echo "explicit_parent: accepted\n";
} catch (ValueError $e) {
    echo "explicit_parent: rejected\n";
}

$after = $chart->renderSvg();
echo "state_preserved: ", ($before === $after ? 'yes' : 'no'), "\n";
echo "ordinary: ", (simplexml_load_string($after, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false ? 'ok' : 'BAD'), "\n";
?>
--EXPECT--
extreme: FastChart\SunburstChart::setHierarchy() aggregate values exceed the supported numeric range
explicit_parent: rejected
state_preserved: yes
ordinary: ok
