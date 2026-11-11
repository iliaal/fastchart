--TEST--
GaugeChart retains storage for accepted zones rather than the zone limit
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (memory_get_usage() === 0) die('skip Zend memory accounting is disabled');
?>
--FILE--
<?php
$chart = new FastChart\GaugeChart();
$one = [['from' => 0, 'to' => 100, 'color' => 0x123456]];
$full = array_fill(0, 16, $one[0]);
$filtered = array_fill(0, 15, null);
$filtered['accepted'] = $one[0];
$invalid = array_fill(0, 16, null);

function retained(FastChart\GaugeChart $chart, array $zones): int {
    $chart->setZones([]);
    $before = memory_get_usage();
    $chart->setZones($zones);
    return memory_get_usage() - $before;
}

$oneBytes = retained($chart, $one);
$fullBytes = retained($chart, $full);
$filteredBytes = retained($chart, $filtered);
$invalidBytes = retained($chart, $invalid);
$emptyBytes = retained($chart, []);
/* Compare allocation classes, not platform-specific byte counts. */
echo 'one smaller than full: ', $oneBytes > 0 && $oneBytes < $fullBytes ? 'yes' : 'BAD', "\n";
echo 'filtered matches one: ', $filteredBytes === $oneBytes ? 'yes' : 'BAD', "\n";
echo 'no accepted zones retain nothing: ', $invalidBytes === 0 && $emptyBytes === 0 ? 'yes' : 'BAD', "\n";
?>
--EXPECT--
one smaller than full: yes
filtered matches one: yes
no accepted zones retain nothing: yes
