--TEST--
AreaChart: band mode neutralizes the secondary Y axis (both boundaries on the left range)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_2423e27e: with setSecondaryYAxis(true) and a band series flagged
 * 'axis' => 'right', the range loop routed that boundary into the right
 * range while the band draw path mapped both boundaries through the left
 * range — the right-flagged boundary flat-lined against a plot edge and
 * an orphan right axis was drawn. Both-right threw despite valid data.
 * Band mode now ignores the secondary axis like stream mode does. */

use FastChart\AreaChart;

function band(?string $axis0, ?string $axis1, bool $secondary): string {
    $upper = ['data' => [12.5, 14.5, 13.5, 16.5, 15.5]];
    $lower = ['data' => [7.5, 9.5, 8.5, 11.5, 10.5]];
    if ($axis0) $upper['axis'] = $axis0;
    if ($axis1) $lower['axis'] = $axis1;
    $c = (new AreaChart(400, 250))
        ->setStacked(false)
        ->setSeries([$upper, $lower])
        ->setBandMode(true);
    if ($secondary) $c->setSecondaryYAxis(true);
    return $c->renderSvg();
}

$plain = band(null, null, false);
echo "plain has polygon: ", str_contains($plain, '<polygon') ? 'yes' : 'NO', "\n";

/* setSecondaryYAxis alone still reserves the right margin at layout
 * time (same accepted behavior as stream mode), so compare against the
 * same-layout no-flag chart: the per-series right flag must not change
 * the geometry. */
$sec_plain = band(null, null, true);
echo "right_flag_ignored: ",
    (band(null, 'right', true) === $sec_plain ? 'yes' : 'NO'), "\n";

/* Both boundaries right-flagged: no "found no numeric values" throw,
 * same band. */
try {
    echo "both_right_ok: ",
        (band('right', 'right', true) === $sec_plain ? 'yes' : 'NO'), "\n";
} catch (Throwable $e) {
    echo "both_right_ok: THREW (", $e->getMessage(), ")\n";
}

?>
--EXPECT--
plain has polygon: yes
right_flag_ignored: yes
both_right_ok: yes
