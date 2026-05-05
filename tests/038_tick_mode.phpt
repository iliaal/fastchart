--TEST--
setTickMode: TICK_NONE / TICK_LABELS / TICK_POINTS / TICK_BOTH
--EXTENSIONS--
fastchart
--FILE--
<?php

$render = function ($mode) {
    return (new FastChart\LineChart(500, 400))
        ->setTickMode($mode)
        ->setSeries([1, 5, 3, 8, 4])
        ->renderPng();
};

// All four modes must produce a valid PNG without crashing.
foreach ([
    'NONE'   => FastChart\Chart::TICK_NONE,
    'LABELS' => FastChart\Chart::TICK_LABELS,
    'POINTS' => FastChart\Chart::TICK_POINTS,
    'BOTH'   => FastChart\Chart::TICK_BOTH,
] as $name => $mode) {
    $b = $render($mode);
    echo "$name: ", strlen($b) > 1024 ? "ok" : "bad", "\n";
}

// Out-of-range rejected.
try {
    (new FastChart\LineChart)->setTickMode(99);
    echo "bad: no throw\n";
} catch (\ValueError $e) {
    echo "bad: ValueError ok\n";
}
?>
--EXPECT--
NONE: ok
LABELS: ok
POINTS: ok
BOTH: ok
bad: ValueError ok
