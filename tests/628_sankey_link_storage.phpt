--TEST--
SankeyChart retains only accepted link storage
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (memory_get_usage() === 0) die('skip Zend memory accounting is disabled');
?>
--FILE--
<?php
function retained(FastChart\Chart $chart, array $links): int {
    $chart->setLinks([]);
    $before = memory_get_usage();
    $chart->setLinks($links);
    return memory_get_usage() - $before;
}
$one = [['from' => 0, 'to' => 1, 'value' => 2]];
$full = array_fill(0, 1024, $one[0]);
$filtered = array_fill(0, 1023, null);
$filtered['last'] = $one[0];
$invalid = array_fill(0, 1024, null);
foreach (['SankeyChart'] as $name) {
    $class = 'FastChart\\' . $name;
    $chart = (new $class())->setNodes([[], []]);
    $oneBytes = retained($chart, $one);
    $fullBytes = retained($chart, $full);
    $filteredBytes = retained($chart, $filtered);
    $invalidBytes = retained($chart, $invalid);
    $emptyBytes = retained($chart, []);
    /* Compare allocation classes rather than platform-specific byte counts. */
    echo "$name compact: ", $oneBytes > 0 && $oneBytes < $fullBytes
        && $filteredBytes === $oneBytes ? 'yes' : 'BAD', "\n";
    echo "$name empty: ", $invalidBytes === 0 && $emptyBytes === 0 ? 'yes' : 'BAD', "\n";
}
?>
--EXPECT--
SankeyChart compact: yes
SankeyChart empty: yes
