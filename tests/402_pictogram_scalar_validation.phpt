--TEST--
Pictogram rejects undocumented scalar coercions without changing valid state
--EXTENSIONS--
fastchart
--FILE--
<?php

function rejects(string $label, FastChart\Pictogram $chart, callable $call): void
{
    $before = $chart->renderSvg();
    try {
        $call($chart);
        echo "$label: accepted\n";
    } catch (ValueError $e) {
        echo "$label: ", $chart->renderSvg() === $before ? "preserved\n" : "changed\n";
    }
}

$base = fn () => (new FastChart\Pictogram(240, 120))
    ->setTotal(10)
    ->setValue(5)
    ->setIconCount(10);

rejects('nonfinite value', $base(), fn ($c) => $c->setValue(INF));
rejects('negative columns', $base(), fn ($c) => $c->setColumns(-1));
rejects('excess columns', $base(), fn ($c) => $c->setColumns(1001));

try {
    $base()->setIconCount(0);
    $base()->setIconCount(1001);
    echo "documented icon clamp: accepted\n";
} catch (ValueError $e) {
    echo "documented icon clamp: rejected\n";
}

?>
--EXPECT--
nonfinite value: preserved
negative columns: preserved
excess columns: preserved
documented icon clamp: accepted
