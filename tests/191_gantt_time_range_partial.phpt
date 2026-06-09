--TEST--
GanttChart::setTimeRange(): a null side auto-fits from task data
--EXTENSIONS--
fastchart
--FILE--
<?php

/* The stub promises "pass null for either to auto-fit", but a null
 * side was stored as literal 0 under a single has-range flag:
 * start-only threw "requires start < end" against the placeholder
 * end, and end-only anchored the axis at 1970-01-01. */

const DAY = 86400;
$t0 = 1700000000;            /* 2023-11-14 */
$tasks = [
    ['name' => 'a', 'start' => $t0,       'end' => $t0 + 2 * DAY,
     'color' => 0xCC0022],
    ['name' => 'b', 'start' => $t0 + DAY, 'end' => $t0 + 3 * DAY],
];

/* The red task spans 2 of the ~3-5 days in view; if the axis
 * auto-fits its null side from the tasks, its bar is wide. Anchoring
 * at epoch 0 collapses it to a sliver at the right edge. */

/* Start-only: documented one-arg usage. Pre-fix: ValueError. */
$svg = (new FastChart\GanttChart(600, 300))
    ->setTasks($tasks)
    ->setTimeRange($t0 - DAY)
    ->renderSvg();
preg_match('/<rect[^>]*width="(\d+)"[^>]*fill="#cc0022"/i', $svg, $m);
var_dump(isset($m[1]) && (int) $m[1] >= 50);

/* End-only: axis must not anchor at 1970. */
$svg = (new FastChart\GanttChart(600, 300))
    ->setTasks($tasks)
    ->setTimeRange(null, $t0 + 4 * DAY)
    ->renderSvg();
preg_match('/<rect[^>]*width="(\d+)"[^>]*fill="#cc0022"/i', $svg, $m);
var_dump(isset($m[1]) && (int) $m[1] >= 50);
var_dump(!str_contains($svg, '1970-01-01'));

/* Both-null call resets to full auto-fit. */
$svg = (new FastChart\GanttChart(600, 300))
    ->setTasks($tasks)
    ->setTimeRange($t0, $t0 + DAY)
    ->setTimeRange()
    ->renderSvg();
preg_match('/<rect[^>]*width="(\d+)"[^>]*fill="#cc0022"/i', $svg, $m);
var_dump(isset($m[1]) && (int) $m[1] >= 50);

/* Fully-specified ranges still validate. */
try {
    (new FastChart\GanttChart(600, 300))->setTimeRange(5, 5);
    echo "no error\n";
} catch (ValueError $e) {
    echo $e->getMessage(), "\n";
}

?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
FastChart\GanttChart::setTimeRange() requires start < end
