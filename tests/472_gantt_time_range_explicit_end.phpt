--TEST--
GanttChart::setTimeRange(): an explicit end survives a partial range
--EXTENSIONS--
fastchart
--FILE--
<?php

/* The stub promises a null side auto-fits from task data. When the
 * explicit end preceded every task the shared "t_max <= t_min" guard
 * overwrote it with auto_start + 86400, so setTimeRange(null, -100)
 * drew the axis at 1000..87400 instead of ending at -100. The fix
 * synthesizes only the missing side, so a partial range must now render
 * exactly like the fully-specified range that spells out that
 * synthesis, and must not render like the auto-fit. */

const DAY = 86400;

$chart = new FastChart\GanttChart(600, 300);
$chart->setTasks([
    ['name' => 'a', 'start' => 1000, 'end' => 2000, 'color' => 0xCC0022],
]);

/* The task bar is the only 0xCC0022 rect, so its geometry is the
 * observable of the time window: t_min/t_max drive its x and width. */
function bar(FastChart\GanttChart $chart, ?int $start, ?int $end): string
{
    $svg = $chart->setTimeRange($start, $end)->renderSvg();
    preg_match('/<rect[^>]*fill="#cc0022"[^>]*\/>/i', $svg, $m);
    return $m[0] ?? 'none';
}

/* End before the first task: the caller's end is the right edge and the
 * missing start is synthesized one day earlier. */
$endBeforeTask = bar($chart, null, -100);
var_dump($endBeforeTask === bar($chart, -100 - DAY, -100));
/* ... and that is not the auto-fit window the old guard fell back to. */
var_dump($endBeforeTask !== bar($chart, null, null));

/* End after the last task: unchanged, the null start still auto-fits
 * from task data. */
var_dump(bar($chart, null, 3000) === bar($chart, 1000, 3000));
/* Start-only: unchanged, the null end still auto-fits from task data. */
var_dump(bar($chart, 500, null) === bar($chart, 500, 2000));
/* Fully-specified range: unchanged. */
var_dump(bar($chart, 500, 3000) === bar($chart, 500, 3000));

/* Overflow controls: the synthesized side saturates instead of
 * wrapping, and matches the explicit spelling of the same window. */
var_dump(bar($chart, null, PHP_INT_MAX) === bar($chart, PHP_INT_MAX - DAY, PHP_INT_MAX));
var_dump(bar($chart, null, PHP_INT_MIN + 10) === bar($chart, PHP_INT_MIN, PHP_INT_MIN + 10));
var_dump(bar($chart, PHP_INT_MAX, null) === bar($chart, PHP_INT_MAX - DAY, PHP_INT_MAX));
var_dump(bar($chart, PHP_INT_MIN, null) === bar($chart, PHP_INT_MIN, PHP_INT_MIN + DAY));

/* setTimeRange() still rejects a fully-specified reversed range. */
try {
    $chart->setTimeRange(5, 5);
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
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
FastChart\GanttChart::setTimeRange() requires start < end
