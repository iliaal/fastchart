--TEST--
Over-cap Scatter/Radar/Polar/Network/Arc/Chord setters preserve prior state
--EXTENSIONS--
fastchart
--FILE--
<?php

/* The series-count / node-count caps on these setters are validated
 * BEFORE the previously-committed state is released, so a caught
 * ValueError leaves the chart byte-identical on re-render. Pre-fix the
 * free ran first and the chart came back empty (or rendered nothing). */

function survives(string $label, object $chart, callable $overcap,
                  string $want): void {
    $before = $chart->renderSvg();
    try {
        $overcap($chart);
        echo "$label: NO THROW\n";
        return;
    } catch (\ValueError $e) {
        $msg = $e->getMessage() === $want ? "msg-ok" : "MSG: {$e->getMessage()}";
    }
    $after = $chart->renderSvg();
    echo "$label: ", ($after === $before ? "survives" : "CORRUPTED"),
         " ($msg)\n";
}

survives('scatter',
    (new FastChart\ScatterChart(300, 300))->setPoints([
        ['label' => 's', 'data' => [[1, 2], [3, 4]]]]),
    fn($c) => $c->setPoints(array_fill(0, 9, ['data' => [[0, 0]]])),
    'FastChart\ScatterChart::setPoints() accepts at most 8 series; got 9');

survives('radar',
    (new FastChart\RadarChart(300, 300))->setSeries([
        ['label' => 'a', 'data' => [1, 2, 3]]]),
    fn($c) => $c->setSeries(array_fill(0, 9, ['data' => [1, 2, 3]])),
    'FastChart\RadarChart::setSeries() accepts at most 8 series; got 9');

survives('polar',
    (new FastChart\PolarChart(300, 300))->setSeries([
        ['label' => 'a', 'data' => [[0, 1], [90, 2]]]]),
    fn($c) => $c->setSeries(array_fill(0, 9, ['data' => [[0, 1]]])),
    'FastChart\PolarChart::setSeries() accepts at most 8 series; got 9');

$nodes513 = array_fill(0, 513, ['label' => 'n']);

survives('network',
    (new FastChart\NetworkChart(300, 300))
        ->setNodes([['label' => 'a'], ['label' => 'b']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 1]]),
    fn($c) => $c->setNodes($nodes513),
    'FastChart\NetworkChart::setNodes() accepts at most 512 nodes; got 513');

survives('arc',
    (new FastChart\ArcDiagram(300, 300))
        ->setNodes([['label' => 'a'], ['label' => 'b']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 1]]),
    fn($c) => $c->setNodes($nodes513),
    'FastChart\ArcDiagram::setNodes() accepts at most 512 nodes; got 513');

survives('chord',
    (new FastChart\ChordDiagram(300, 300))
        ->setNodes([['label' => 'a'], ['label' => 'b']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 1]]),
    fn($c) => $c->setNodes($nodes513),
    'FastChart\ChordDiagram::setNodes() accepts at most 512 nodes; got 513');

echo "done\n";
?>
--EXPECT--
scatter: survives (msg-ok)
radar: survives (msg-ok)
polar: survives (msg-ok)
network: survives (msg-ok)
arc: survives (msg-ok)
chord: survives (msg-ok)
done
