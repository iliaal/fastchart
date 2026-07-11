--TEST--
Arc/Chord/Network setNodes/setLinks throw on over-cap instead of truncating
--EXTENSIONS--
fastchart
--FILE--
<?php

/* The 1.6.0 changelog promised fixed-cap data setters throw ValueError
 * rather than silently truncating. The three graph families routed
 * through a shared parse helper that clamped instead. They now reject
 * over-cap input at the setter with an exact count in the message. */

function throws(string $label, callable $fn, string $want): void {
    try {
        $fn();
        echo "$label: NO THROW\n";
    } catch (\ValueError $e) {
        echo "$label: ", $e->getMessage() === $want ? "ok" : $e->getMessage(), "\n";
    }
}

$nodes513 = array_fill(0, 513, ['label' => 'n']);
$twoNodes = [['label' => 'a'], ['label' => 'b']];
$links2049 = array_fill(0, 2049, ['from' => 0, 'to' => 1, 'value' => 1]);

throws('network setNodes(513)',
    fn() => (new FastChart\NetworkChart(300, 300))->setNodes($nodes513),
    'FastChart\NetworkChart::setNodes() accepts at most 512 nodes; got 513');
throws('arc setNodes(513)',
    fn() => (new FastChart\ArcDiagram(300, 300))->setNodes($nodes513),
    'FastChart\ArcDiagram::setNodes() accepts at most 512 nodes; got 513');
throws('chord setNodes(513)',
    fn() => (new FastChart\ChordDiagram(300, 300))->setNodes($nodes513),
    'FastChart\ChordDiagram::setNodes() accepts at most 512 nodes; got 513');

throws('network setLinks(2049)',
    fn() => (new FastChart\NetworkChart(300, 300))->setNodes($twoNodes)->setLinks($links2049),
    'FastChart\NetworkChart::setLinks() accepts at most 2048 links; got 2049');
throws('arc setLinks(2049)',
    fn() => (new FastChart\ArcDiagram(300, 300))->setNodes($twoNodes)->setLinks($links2049),
    'FastChart\ArcDiagram::setLinks() accepts at most 2048 links; got 2049');
throws('chord setLinks(2049)',
    fn() => (new FastChart\ChordDiagram(300, 300))->setNodes($twoNodes)->setLinks($links2049),
    'FastChart\ChordDiagram::setLinks() accepts at most 2048 links; got 2049');

// At-cap input is accepted (512 nodes / 2048 links).
$nodes512 = array_fill(0, 512, ['label' => 'n']);
$links2048 = array_fill(0, 2048, ['from' => 0, 'to' => 1, 'value' => 1]);
$net = (new FastChart\NetworkChart(300, 300))->setNodes($nodes512)->setLinks($links2048);
echo "at-cap accepted: ", strlen($net->renderSvg()) > 0 ? "ok" : "FAIL", "\n";

echo "done\n";
?>
--EXPECT--
network setNodes(513): ok
arc setNodes(513): ok
chord setNodes(513): ok
network setLinks(2049): ok
arc setLinks(2049): ok
chord setLinks(2049): ok
at-cap accepted: ok
done
