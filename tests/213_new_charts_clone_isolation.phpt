--TEST--
New charts: clone deep-copies heap data and outlives the original
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* The graph / hierarchy / string-array / nested-array backed charts all
 * own heap allocations (node + link arrays, hierarchy trees, label and
 * value buffers). A shallow clone would alias those pointers and
 * double-free or read freed memory once one copy is destroyed. Build
 * each chart, clone it, free the original, and render the clone: under
 * the ASAN test build a shared pointer would crash here. The clone must
 * also render byte-identical to an independent fresh build, proving the
 * copy is faithful, not just non-crashing. */

function valid(string $svg): bool {
    return strlen($svg) > 100 &&
        simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

$builders = [
    'ArcDiagram' => fn() => (new FastChart\ArcDiagram(400, 200))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 2],
                    ['from' => 1, 'to' => 2, 'value' => 3]]),
    'ChordDiagram' => fn() => (new FastChart\ChordDiagram(300, 300))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 2],
                    ['from' => 1, 'to' => 2, 'value' => 3]]),
    'NetworkChart' => fn() => (new FastChart\NetworkChart(300, 300))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 2],
                    ['from' => 1, 'to' => 2, 'value' => 3]]),
    'PopulationPyramid' => fn() => (new FastChart\PopulationPyramid(300, 300))
        ->setCategories(['a', 'b', 'c'])
        ->setLeftSeries(['label' => 'L', 'data' => [1, 2, 3]])
        ->setRightSeries(['label' => 'R', 'data' => [2, 3, 1]]),
    'ViolinPlot' => fn() => (new FastChart\ViolinPlot(300, 300))
        ->setGroups([['label' => 'X', 'values' => [1, 2, 3, 4, 3, 2]],
                     ['label' => 'Y', 'values' => [2, 4, 6, 4, 2]]]),
    'CirclePacking' => fn() => (new FastChart\CirclePacking(300, 300))
        ->setHierarchy(['children' => [
            ['label' => 'a', 'value' => 5], ['value' => 3], ['value' => 8]]]),
    'VennDiagram' => fn() => (new FastChart\VennDiagram(300, 300))
        ->setSets([['label' => 'A', 'size' => 10], ['label' => 'B', 'size' => 8]])
        ->setIntersections([['sets' => [0, 1], 'size' => 3]]),
    'WordCloud' => fn() => (new FastChart\WordCloud(300, 300))
        ->setWords([['text' => 'alpha', 'weight' => 5],
                    ['text' => 'beta', 'weight' => 3],
                    ['text' => 'gamma', 'weight' => 8]]),
    'SerpentineTimeline' => fn() => (new FastChart\SerpentineTimeline(400, 200))
        ->setEvents([['label' => 'a', 'date' => 'Jan'],
                     ['label' => 'b', 'date' => 'Feb'],
                     ['label' => 'c', 'date' => 'Mar']]),
    'Dendrogram' => fn() => (new FastChart\Dendrogram(300, 300))
        ->setHierarchy(['children' => [
            ['label' => 'a', 'value' => 5], ['value' => 3], ['value' => 8]]]),
    'Partition' => fn() => (new FastChart\Partition(300, 300))
        ->setHierarchy(['children' => [
            ['label' => 'a', 'value' => 5], ['value' => 3], ['value' => 8]]]),
];

foreach ($builders as $name => $build) {
    $ref  = $build()->renderSvg();   /* independent reference render */
    $orig = $build();
    $copy = clone $orig;
    unset($orig);                    /* free the original's heap */
    $out = $copy->renderSvg();       /* clone must still own its data */
    echo "$name: ", (valid($out) && $out === $ref ? "ok" : "BAD"), "\n";
}
echo "done\n";
?>
--EXPECT--
ArcDiagram: ok
ChordDiagram: ok
NetworkChart: ok
PopulationPyramid: ok
ViolinPlot: ok
CirclePacking: ok
VennDiagram: ok
WordCloud: ok
SerpentineTimeline: ok
Dendrogram: ok
Partition: ok
done
